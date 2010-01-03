/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

/* relay-protocol-irc.c: IRC protocol for client
                         (relay acting as an IRC proxy/bouncer) */


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
#include "relay-protocol-irc.h"
#include "relay-client.h"


/*
 * relay_protocol_irc_search_buffer: search IRC buffer with server and channel
 *                                   name
 */

struct t_gui_buffer *
relay_protocol_irc_search_buffer (const char *server, const char *channel)
{
    char string[256];
    long unsigned int value;
    const char *str_ptr_buffer;
    
    snprintf (string, sizeof (string), "%s,%s", server, channel);
    str_ptr_buffer = weechat_info_get ("irc_buffer", string);
    if (str_ptr_buffer && str_ptr_buffer[0])
    {
        sscanf (str_ptr_buffer, "%lx", &value);
        return (struct t_gui_buffer *)value;
    }
    
    return NULL;
}

/*
 * relay_protocol_irc_sendf: send formatted data to client
 */

int
relay_protocol_irc_sendf (struct t_relay_client *client, const char *format, ...)
{
    va_list args;
    static char buffer[4096];
    int length, num_sent;
    
    if (!client)
        return 0;
    
    va_start (args, format);
    vsnprintf (buffer, sizeof (buffer) - 3, format, args);
    va_end (args);

    if (weechat_relay_plugin->debug)
    {
        weechat_printf (NULL, "relay: send: %s", buffer);
    }
    
    length = strlen (buffer);
    
    buffer[length] = '\r';
    buffer[length + 1] = '\n';
    buffer[length + 2] = '\0';
    length += 2;
    
    num_sent = send (client->sock, buffer, length, 0);
    
    if (num_sent >= 0)
        client->bytes_sent += num_sent;
    else
    {
        weechat_printf (NULL,
                        _("%s%s: error sending data to client %s"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        strerror (errno));
    }
    
    return num_sent;
}

/*
 * relay_protocol_irc_signal_irc_in2_cb: callback for "irc_in2" IRC signal
 */

int
relay_protocol_irc_signal_irc_in2_cb (void *data, const char *signal,
                                      const char *type_data, void *signal_data)
{
    struct t_relay_client *client;
    const char *ptr_msg;
    char *host, *pos, *pos_end_nick;
    
    (void) signal;
    (void) type_data;
    
    client = (struct t_relay_client *)data;
    ptr_msg = (const char *)signal_data;
    
    if (weechat_relay_plugin->debug)
    {
        weechat_printf (NULL, "relay: irc_in2: client: %s, data: %s",
                        client->protocol_string,
                        ptr_msg);
    }
    
    if (ptr_msg[0] == ':')
    {
        pos = strchr (ptr_msg, ' ');
        if (pos)
        {
            host = weechat_strndup (ptr_msg + 1, pos - ptr_msg);
            if (host)
            {
                pos_end_nick = strchr (host, '!');
                if (pos_end_nick)
                    pos_end_nick[0] = '\0';
                else
                    host[0] = '\0';
                
                pos++;
                while (pos[0] == ' ')
                {
                    pos++;
                }
                
                relay_protocol_irc_sendf (client, ":%s%s%s %s",
                                          (host[0]) ? host : "",
                                          (host[0]) ? "!" : "",
                                          RELAY_IRC_DATA(client, address),
                                          pos);
                free (host);
            }
            return WEECHAT_RC_OK;
        }
    }
    
    relay_protocol_irc_sendf (client, "%s", ptr_msg);
    
    return WEECHAT_RC_OK;
}

/*
 * relay_protocol_irc_signal_irc_out_cb: callback for "irc_out" IRC signal
 */

int
relay_protocol_irc_signal_irc_out_cb (void *data, const char *signal,
                                      const char *type_data, void *signal_data)
{
    struct t_relay_client *client;
    
    (void) signal;
    (void) type_data;
    
    client = (struct t_relay_client *)data;
    
    if (weechat_relay_plugin->debug)
    {
        weechat_printf (NULL, "relay: irc_out: client: %s, data: %s",
                        client->protocol_string,
                        (char *)signal_data);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * relay_protocol_irc_send_join: send join for a channel to client
 */

void
relay_protocol_irc_send_join (struct t_relay_client *client,
                              const char *channel)
{
    char *infolist_name, *nicks;
    const char *nick;
    int length, length_nicks;
    struct t_infolist *infolist_nicks;
    
    length = strlen (client->protocol_string) + 1 + strlen (channel) + 1;
    infolist_name = malloc (length);
    if (infolist_name)
    {
        relay_protocol_irc_sendf (client,
                                  ":%s!%s@proxy JOIN %s",
                                  RELAY_IRC_DATA(client, nick),
                                  "weechat",
                                  channel);
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
                if (nick && nick[0])
                {
                    if (length_nicks == 0)
                    {
                        length_nicks = strlen (nick) + 1;
                        nicks = malloc (length_nicks);
                        strcpy (nicks, nick);
                    }
                    else
                    {
                        length_nicks += strlen (nick) + 1;
                        nicks = realloc (nicks, length_nicks);
                        strcat (nicks, " ");
                        strcat (nicks, nick);
                    }
                }
            }
            if (nicks)
            {
                relay_protocol_irc_sendf (client,
                                          ":%s 353 %s = %s :%s",
                                          RELAY_IRC_DATA(client, address),
                                          RELAY_IRC_DATA(client, nick),
                                          channel, nicks);
                free (nicks);
            }
            weechat_infolist_free (infolist_nicks);
        }
        relay_protocol_irc_sendf (client,
                                  ":%s 366 %s %s :End of /NAMES list.",
                                  RELAY_IRC_DATA(client, address),
                                  RELAY_IRC_DATA(client, nick),
                                  channel);
        free (infolist_name);
    }
}

/*
 * relay_protocol_irc_send_join_channels: send join for all channels of server
 *                                        to client
 */

void
relay_protocol_irc_send_join_channels (struct t_relay_client *client)
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
            relay_protocol_irc_send_join (client, channel);
        }
        weechat_infolist_free (infolist_channels);
    }
}

/*
 * relay_protocol_irc_recv_one_msg: read one message from client
 */

void
relay_protocol_irc_recv_one_msg (struct t_relay_client *client, char *data)
{
    char *pos, str_time[128], **argv, **argv_eol, str_signal_name[128];
    char *command;
    int argc, length;
    const char *nick;
    struct t_gui_buffer *ptr_buffer;
    
    pos = strchr (data, '\r');
    if (pos)
        pos[0] = '\0';
    
    if (weechat_relay_plugin->debug)
    {
        weechat_printf (NULL, "relay: recv from client: \"%s\"", data);
    }
    
    argv = weechat_string_split (data, " ", 0, 0, &argc);
    argv_eol = weechat_string_split (data, " ", 1, 0, &argc);
    
    if (!RELAY_IRC_DATA(client, connected))
    {
        if (weechat_strncasecmp (data, "nick ", 5) == 0)
        {
            if (data[5])
            {
                if (RELAY_IRC_DATA(client, nick))
                    free (RELAY_IRC_DATA(client, nick));
                RELAY_IRC_DATA(client, nick) = strdup (data + 5);
            }
        }
        if (weechat_strncasecmp (data, "user ", 5) == 0)
        {
            if (data[5])
            {
                RELAY_IRC_DATA(client, user_received) = 1;
            }
        }
        if (RELAY_IRC_DATA(client, nick) && RELAY_IRC_DATA(client, user_received))
        {
            RELAY_IRC_DATA(client, connected) = 1;
            
            /* send nick to client if server nick is different of nick asked
               by client with command NICK */
            nick = weechat_info_get ("irc_nick", client->protocol_string);
            if (nick && (strcmp (nick, RELAY_IRC_DATA(client, nick)) != 0))
            {
                relay_protocol_irc_sendf (client,
                                          ":%s!proxy NICK :%s",
                                          RELAY_IRC_DATA(client, nick),
                                          nick);
                free (RELAY_IRC_DATA(client, nick));
                RELAY_IRC_DATA(client, nick) = strdup (nick);
            }
            
            relay_protocol_irc_sendf (client,
                                      ":%s 001 %s :Welcome to the Internet "
                                      "Relay Network %s!%s@proxy",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick),
                                      RELAY_IRC_DATA(client, nick),
                                      "weechat");
            relay_protocol_irc_sendf (client,
                                      ":%s 002 %s :Your host is "
                                      "weechat-relay-irc, running version %s",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick),
                                      weechat_info_get("version", NULL));
            snprintf (str_time, sizeof (str_time), "%s",
                      ctime (&client->listen_start_time));
            if (str_time[0])
                str_time[strlen (str_time) - 1] = '\0';
            relay_protocol_irc_sendf (client,
                                      ":%s 003 %s :This server was created "
                                      "on %s",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick),
                                      str_time);
            relay_protocol_irc_sendf (client,
                                      ":%s 004 %s %s %s oirw abiklmnopqstv",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick),
                                      RELAY_IRC_DATA(client, address),
                                      weechat_info_get("version", NULL));
            relay_protocol_irc_sendf (client,
                                      ":%s 251 %s :There are %d users and 0 "
                                      "invisible on 1 servers",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick),
                                      relay_client_count);
            relay_protocol_irc_sendf (client,
                                      ":%s 255 %s :I have %d clients, 0 "
                                      "services and 0 servers",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick),
                                      relay_client_count);
            relay_protocol_irc_sendf (client,
                                      ":%s 422 %s :MOTD File is missing",
                                      RELAY_IRC_DATA(client, address),
                                      RELAY_IRC_DATA(client, nick));
            
            /* hook signal "xxx,irc_in2_*" to catch IRC data received from
               this server */
            snprintf (str_signal_name, sizeof (str_signal_name),
                      "%s,irc_in2_*",
                      client->protocol_string);
            RELAY_IRC_DATA(client, hook_signal_irc_in2) =
                weechat_hook_signal (str_signal_name,
                                     &relay_protocol_irc_signal_irc_in2_cb,
                                     client);

            /* hook signal "xxx,irc_out_*" to catch IRC data sent to
               this server */
            snprintf (str_signal_name, sizeof (str_signal_name),
                      "%s,irc_out_*",
                      client->protocol_string);
            RELAY_IRC_DATA(client, hook_signal_irc_out) =
                weechat_hook_signal (str_signal_name,
                                     &relay_protocol_irc_signal_irc_out_cb,
                                     client);
            
            /* send JOIN for all channels on server to client */
            relay_protocol_irc_send_join_channels (client);
        }
    }
    else
    {
        if (argc > 0)
        {
            if (weechat_strcasecmp (argv[0], "privmsg") == 0)
            {
                ptr_buffer = relay_protocol_irc_search_buffer (client->protocol_string,
                                                               argv[1]);
                if (ptr_buffer)
                {
                    weechat_printf (NULL,
                                    "relay: send string \"%s\" on channel %s",
                                    (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2],
                                    argv[1]);
                    weechat_command (ptr_buffer,
                                     (argv_eol[2][0] == ':') ? argv_eol[2] + 1 : argv_eol[2]);
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: buffer not found for IRC server "
                                      "\"%s\", channel \"%s\""),
                                    weechat_prefix ("error"),
                                    RELAY_PLUGIN_NAME,
                                    client->protocol_string,
                                    argv[1]);
                }
            }
            else
            {
                length = 32 + strlen (client->protocol_string) + strlen (data);
                command = malloc (length + 1);
                if (command)
                {
                    snprintf (command, length, "/quote -server %s %s",
                              client->protocol_string,
                              data);
                    weechat_command (NULL, command);
                    free (command);
                }
            }
        }
    }

    if (argv)
        weechat_string_free_split (argv);
    if (argv_eol)
        weechat_string_free_split (argv_eol);
}

/*
 * relay_protocol_irc_recv: read data from client
 */

void
relay_protocol_irc_recv (struct t_relay_client *client, const char *data)
{
    char **items;
    int items_count, i;
    
    items = weechat_string_split (data, "\n", 0, 0, &items_count);
    for (i = 0; i < items_count; i++)
    {
        relay_protocol_irc_recv_one_msg (client, items[i]);
    }
    if (items)
        weechat_string_free_split (items);
}

/*
 * relay_protocol_irc_alloc: init relay data specific to IRC protocol
 */

void
relay_protocol_irc_alloc (struct t_relay_client *client)
{
    struct t_relay_protocol_irc_data *irc_data;
    
    client->protocol_data = malloc (sizeof (*irc_data));
    if (client->protocol_data)
    {
        RELAY_IRC_DATA(client, address) = strdup ("weechat.relay.irc");
        RELAY_IRC_DATA(client, nick) = NULL;
        RELAY_IRC_DATA(client, user_received) = 0;
        RELAY_IRC_DATA(client, connected) = 0;
        RELAY_IRC_DATA(client, hook_signal_irc_in2) = NULL;
        RELAY_IRC_DATA(client, hook_signal_irc_out) = NULL;
    }
}

/*
 * relay_protocol_irc_free: free relay data specific to IRC protocol
 */

void
relay_protocol_irc_free (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        if (RELAY_IRC_DATA(client, address))
            free (RELAY_IRC_DATA(client, address));
        if (RELAY_IRC_DATA(client, nick))
            free (RELAY_IRC_DATA(client, nick));
        if (RELAY_IRC_DATA(client, hook_signal_irc_in2))
            weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_in2));
        if (RELAY_IRC_DATA(client, hook_signal_irc_out))
            weechat_unhook (RELAY_IRC_DATA(client, hook_signal_irc_out));
        
        free (client->protocol_data);
    }
}

/*
 * relay_protocol_irc_print_log: print IRC client infos in log (usually for
 *                               crash dump)
 */

void
relay_protocol_irc_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        weechat_log_printf ("    address. . . . . . : '%s'",  RELAY_IRC_DATA(client, address));
        weechat_log_printf ("    nick . . . . . . . : '%s'",  RELAY_IRC_DATA(client, nick));
        weechat_log_printf ("    user_received. . . : %d",    RELAY_IRC_DATA(client, user_received));
        weechat_log_printf ("    connected. . . . . : %d",    RELAY_IRC_DATA(client, connected));
        weechat_log_printf ("    hook_signal_irc_in2: 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_in2));
        weechat_log_printf ("    hook_signal_irc_out: 0x%lx", RELAY_IRC_DATA(client, hook_signal_irc_out));
    }
}
