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

/* irc-server.c: connection and communication with IRC server */


#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-server.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-debug.h"
#include "irc-nick.h"
#include "irc-protocol.h"


struct t_irc_server *irc_servers = NULL;
struct t_irc_server *last_irc_server = NULL;

/* current server when there is one buffer for all servers */
struct t_irc_server *irc_current_server = NULL;

struct t_irc_message *irc_recv_msgq = NULL;
struct t_irc_message *irc_msgq_last_msg = NULL;


/*
 * irc_server_valid: check if a server pointer exists
 *                   return 1 if server exists
 *                          0 if server is not found
 */

int
irc_server_valid (struct t_irc_server *server)
{
    struct t_irc_server *ptr_server;
    
    if (!server)
        return 0;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server == server)
            return 1;
    }
    
    /* server not found */
    return 0;
}

/*
 * irc_server_get_name_without_port: get name of server without port
 *                                   (ends before first '/' if found)
 */

char *
irc_server_get_name_without_port (const char *name)
{
    char *pos;
    
    if (!name)
        return NULL;
    
    pos = strchr (name, '/');
    if (pos && (pos != name))
        return weechat_strndup (name, pos - name);
    
    return strdup (name);
}

/*
 * irc_server_new_option: create option in configuration for a server option
 */

void
irc_server_new_option (struct t_irc_server *server, int index_option,
                       const char *value)
{
    int length;
    char *option_name;
    
    length = strlen (server->name) + 1 +
        strlen (irc_config_server_option_string[index_option]) + 1;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.%s",
                  server->name,
                  irc_config_server_option_string[index_option]);
        irc_config_server_new_option (irc_config_file,
                                      irc_config_section_server,
                                      index_option,
                                      option_name,
                                      value,
                                      &irc_config_server_change_cb,
                                      irc_config_server_option_string[index_option],
                                      &irc_config_server_delete_cb,
                                      irc_config_server_option_string[index_option]);
    }
}

/*
 * irc_server_set_addresses: set addresses for server
 */

void
irc_server_set_addresses (struct t_irc_server *server, const char *addresses)
{
    int i;
    char *pos, *error;
    long number;
    
    /* free data */
    if (server->addresses)
    {
        free (server->addresses);
        server->addresses = NULL;
    }
    server->addresses_count = 0;
    if (server->addresses_array)
    {
        weechat_string_free_exploded (server->addresses_array);
        server->addresses_array = NULL;
    }
    if (server->ports_array)
    {
        free (server->ports_array);
        server->ports_array = NULL;
    }
    
    /* set new address */
    if (addresses && addresses[0])
    {
        server->addresses = strdup (addresses);
        if (server->addresses)
        {
            server->addresses_array = weechat_string_explode (server->addresses,
                                                              ",", 0, 0,
                                                              &server->addresses_count);
            server->ports_array = malloc (server->addresses_count * sizeof (server->ports_array[0]));
            for (i = 0; i < server->addresses_count; i++)
            {
                pos = strchr (server->addresses_array[i], '/');
                if (pos)
                {
                    pos[0] = 0;
                    pos++;
                    error = NULL;
                    number = strtol (pos, &error, 10);
                    server->ports_array[i] = (error && !error[0]) ?
                        number : IRC_SERVER_DEFAULT_PORT;
                }
                else
                {
                    server->ports_array[i] = IRC_SERVER_DEFAULT_PORT;
                }
            }
        }
    }
}

/*
 * irc_server_set_nicks: set nicks for server
 */

void
irc_server_set_nicks (struct t_irc_server *server, const char *nicks)
{
    /* free data */
    if (server->nicks)
    {
        free (server->nicks);
        server->nicks = NULL;
    }
    server->nicks_count = 0;
    if (server->nicks_array)
    {
        weechat_string_free_exploded (server->nicks_array);
        server->nicks_array = NULL;
    }
    
    /* set new nicks */
    server->nicks = strdup ((nicks) ? nicks : IRC_SERVER_DEFAULT_NICKS);
    if (server->nicks)
    {
        server->nicks_array = weechat_string_explode (server->nicks,
                                                      ",", 0, 0,
                                                      &server->nicks_count);
    }
}

/*
 * irc_server_set_with_option: set a server option with a config option
 */

void
irc_server_set_with_option (struct t_irc_server *server,
                            int index_option,
                            struct t_config_option *option)
{
    if (!server || !option)
        return;
    
    switch (index_option)
    {
        case IRC_CONFIG_SERVER_AUTOCONNECT:
            server->autoconnect = weechat_config_integer (option);
            break;
        case IRC_CONFIG_SERVER_AUTORECONNECT:
            server->autoreconnect = weechat_config_integer (option);
            break;
        case IRC_CONFIG_SERVER_AUTORECONNECT_DELAY:
            server->autoreconnect_delay = weechat_config_integer (option);
            break;
        case IRC_CONFIG_SERVER_PROXY:
            if (server->proxy)
                free (server->proxy);
            server->proxy = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_ADDRESSES:
            irc_server_set_addresses (server, weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_IPV6:
            server->ipv6 = weechat_config_integer (option);
            break;
        case IRC_CONFIG_SERVER_SSL:
            server->ssl = weechat_config_integer (option);
            break;
        case IRC_CONFIG_SERVER_PASSWORD:
            if (server->password)
                free (server->password);
            server->password = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_NICKS:
            irc_server_set_nicks (server, weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_USERNAME:
            if (server->username)
                free (server->username);
            server->username = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_REALNAME:
            if (server->realname)
                free (server->realname);
            server->realname = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_LOCAL_HOSTNAME:
            if (server->local_hostname)
                free (server->local_hostname);
            server->local_hostname = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_COMMAND:
            if (server->command)
                free (server->command);
            server->command = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_COMMAND_DELAY:
            server->command_delay = weechat_config_integer (option);
            break;
        case IRC_CONFIG_SERVER_AUTOJOIN:
            if (server->autojoin)
                free (server->autojoin);
            server->autojoin = strdup (weechat_config_string (option));
            break;
        case IRC_CONFIG_SERVER_AUTOREJOIN:
            server->autorejoin = weechat_config_integer (option);
            break;
        case IRC_CONFIG_NUM_SERVER_OPTIONS:
            break;
    }
}

/*
 * irc_server_set_nick: set nickname for a server
 */

void
irc_server_set_nick (struct t_irc_server *server, const char *nick)
{
    struct t_irc_channel *ptr_channel;
    
    if (server->nick)
        free (server->nick);
    server->nick = (nick) ? strdup (nick) : NULL;
    
    weechat_buffer_set (server->buffer, "highlight_words", nick);
    
    /* set local variable "nick" for server and all channels/pv */
    weechat_buffer_set (server->buffer, "localvar_set_nick", nick);
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        weechat_buffer_set (ptr_channel->buffer, "localvar_set_nick", nick);
    }
    
    weechat_bar_item_update ("input_prompt");
}

/*
 * irc_server_init: init server struct with default values
 */

void
irc_server_init (struct t_irc_server *server)
{
    int i;
    
    /* user choices */
    server->name = NULL;
    server->autoconnect = IRC_CONFIG_SERVER_DEFAULT_AUTOCONNECT;
    server->autoreconnect = IRC_CONFIG_SERVER_DEFAULT_AUTORECONNECT;
    server->autoreconnect_delay = IRC_CONFIG_SERVER_DEFAULT_AUTORECONNECT_DELAY;
    server->proxy = NULL;
    server->addresses = NULL;
    server->ipv6 = IRC_CONFIG_SERVER_DEFAULT_IPV6;
    server->ssl = IRC_CONFIG_SERVER_DEFAULT_SSL;
    server->password = NULL;
    server->nicks = NULL;
    server->username = NULL;
    server->realname = NULL;
    server->local_hostname = NULL;
    server->command = NULL;
    server->command_delay = IRC_CONFIG_SERVER_DEFAULT_COMMAND_DELAY;
    server->autojoin = NULL;
    server->autorejoin = IRC_CONFIG_SERVER_DEFAULT_AUTOREJOIN;
    
    /* internal vars */
    server->reloaded_from_config = 0;
    server->addresses_count = 0;
    server->addresses_array = NULL;
    server->ports_array = NULL;
    server->current_address = 0;
    server->sock = -1;
    server->hook_connect = NULL;
    server->hook_fd = NULL;
    server->is_connected = 0;
    server->ssl_connected = 0;
    server->unterminated_message = NULL;
    server->nicks_count = 0;
    server->nicks_array = NULL;
    server->nick = NULL;
    server->nick_modes = NULL;
    server->prefix = NULL;
    server->reconnect_start = 0;
    server->command_time = 0;
    server->reconnect_join = 0;
    server->disable_autojoin = 0;
    server->is_away = 0;
    server->away_message = NULL;
    server->away_time = 0;
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    server->cmd_list_regexp = NULL;
    server->queue_msg = 0;
    server->last_user_message = 0;
    server->outqueue = NULL;
    server->last_outqueue = NULL;
    server->buffer = NULL;
    server->buffer_as_string = NULL;
    server->channels = NULL;
    server->last_channel = NULL;

    /* init with default values from config */
    for (i = 0; i < IRC_CONFIG_NUM_SERVER_OPTIONS; i++)
    {
        irc_server_set_with_option (server, i,
                                    irc_config_server_default[i]);
    }
}

/*
 * irc_server_alloc: allocate a new server and add it to the servers queue
 */

struct t_irc_server *
irc_server_alloc (const char *name)
{
    struct t_irc_server *new_server;

    if (irc_server_search (name))
        return NULL;
    
    /* alloc memory for new server */
    if ((new_server = malloc (sizeof (*new_server))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: error when allocating new server"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }
    
    /* initialize new server */
    irc_server_init (new_server);
    
    /* set name */
    new_server->name = strdup (name);
    
    /* add new server to queue */
    new_server->prev_server = last_irc_server;
    new_server->next_server = NULL;
    if (irc_servers)
        last_irc_server->next_server = new_server;
    else
        irc_servers = new_server;
    last_irc_server = new_server;

    /* all is ok, return address of new server */
    return new_server;
}

/*
 * irc_server_alloc_with_url: init a server with url of this form:
 *                            irc://nick:pass@irc.toto.org:6667
 *                            returns: 1 = ok
 *                                     0 = error
 */

int
irc_server_alloc_with_url (const char *irc_url)
{
    char *irc_url2, *url, *pos_server, *pos_channel, *pos, *pos2;
    char *password, *nick1, *nicks, *autojoin;
    char *default_nicks, *default_password, *default_autojoin, value[16];
    char *server_name;
    int ipv6, ssl, default_ipv6, default_ssl, length;
    struct t_irc_server *ptr_server;
    
    irc_url2 = strdup (irc_url);
    if (!irc_url2)
        return 0;
    
    ipv6 = 0;
    ssl = 0;
    password = NULL;
    nick1 = NULL;
    autojoin = NULL;
    
    if (weechat_strncasecmp (irc_url2, "irc6://", 7) == 0)
    {
        pos = irc_url2 + 7;
        ipv6 = 1;
    }
    else if (weechat_strncasecmp (irc_url2, "ircs://", 7) == 0)
    {
        pos = irc_url2 + 7;
        ssl = 1;
    }
    else if ((weechat_strncasecmp (irc_url2, "irc6s://", 8) == 0)
        || (weechat_strncasecmp (irc_url2, "ircs6://", 8) == 0))
    {
        pos = irc_url2 + 8;
        ipv6 = 1;
        ssl = 1;
    }
    else if (weechat_strncasecmp (irc_url2, "irc://", 6) == 0)
    {
        pos = irc_url2 + 6;
    }
    else
    {
        free (irc_url2);
        return 0;
    }
    
    free (irc_url2);
    
    url = strdup (pos);
    pos_server = strchr (url, '@');
    if (pos_server)
    {
        pos_server[0] = '\0';
        pos_server++;
        if (!pos[0])
        {
            free (url);
            return 0;
        }
        pos2 = strchr (url, ':');
        if (pos2)
        {
            pos2[0] = '\0';
            password = strdup (pos2 + 1);
        }
        nick1 = strdup (url);
    }
    else
        pos_server = url;
    
    if (!pos_server[0])
    {
        free (url);
        return 0;
    }
    pos_channel = strstr (pos_server, "//");
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel += 2;
    }
    if (pos_channel && pos_channel[0])
    {
        if (irc_channel_is_channel (pos_channel))
            autojoin = strdup (pos_channel);
        else
        {
            autojoin = malloc (strlen (pos_channel) + 2);
            strcpy (autojoin, "#");
            strcat (autojoin, pos_channel);
        }
    }

    /* server name ends before first '/' (if found) */
    server_name = irc_server_get_name_without_port (pos_server);
    ptr_server = irc_server_alloc (server_name);
    if (server_name)
        free (server_name);
    
    if (ptr_server)
    {
        irc_server_set_addresses (ptr_server, pos_server);
        
        default_ipv6 = ptr_server->ipv6;
        default_ssl = ptr_server->ssl;
        default_nicks = (ptr_server->nicks) ? strdup (ptr_server->nicks) : NULL;
        default_password = (ptr_server->password) ? strdup (ptr_server->password) : NULL;
        default_autojoin = (ptr_server->autojoin) ? strdup (ptr_server->autojoin) : NULL;
        
        ptr_server->ipv6 = ipv6;
        ptr_server->ssl = ssl;
        
        if (nick1)
        {
            length = ((strlen (nick1) + 2) * 5) + 1;
            nicks = malloc (length);
            if (nicks)
            {
                snprintf (nicks, length,
                          "%s,%s1,%s2,%s3,%s4",
                          nick1, nick1, nick1, nick1, nick1);
                irc_server_set_nicks (ptr_server, nicks);
                free (nicks);
            }
        }
        ptr_server->password = (password) ? strdup (password) : NULL;
        ptr_server->autojoin = (autojoin) ? strdup (autojoin) : NULL;
        ptr_server->autoconnect = 1;
        
        /* create server options */
        irc_server_new_option (ptr_server,
                               IRC_CONFIG_SERVER_ADDRESSES,
                               ptr_server->addresses);
        if (default_ipv6 != ptr_server->ipv6)
        {
            snprintf (value, sizeof (value),
                      "%s",
                      (ptr_server->ipv6) ? "on" : "off");
            irc_server_new_option (ptr_server,
                                   IRC_CONFIG_SERVER_IPV6,
                                   value);
        }
        if (default_ssl != ptr_server->ssl)
        {
            snprintf (value, sizeof (value),
                      "%s",
                      (ptr_server->ssl) ? "on" : "off");
            irc_server_new_option (ptr_server,
                                   IRC_CONFIG_SERVER_SSL,
                                   value);
        }
        if (ptr_server->nicks
            && (!default_nicks || strcmp (default_nicks, ptr_server->nicks) != 0))
        {
            irc_server_new_option (ptr_server,
                                   IRC_CONFIG_SERVER_NICKS,
                                   ptr_server->nicks);
        }
        if (ptr_server->password
            && (!default_password || strcmp (default_password, ptr_server->password) != 0))
        {
            irc_server_new_option (ptr_server,
                                   IRC_CONFIG_SERVER_PASSWORD,
                                   ptr_server->password);
        }
        if (ptr_server->autojoin
            && (!default_autojoin || strcmp (default_autojoin, ptr_server->autojoin) != 0))
        {
            irc_server_new_option (ptr_server,
                                   IRC_CONFIG_SERVER_AUTOJOIN,
                                   ptr_server->autojoin);
        }
    }
    else
    {
        weechat_printf (NULL,
                        _("%s%s: error creating new server \"%s\""),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        pos_server);
    }
    
    if (password)
        free (password);
    if (nick1)
        free (nick1);
    if (autojoin)
        free (autojoin);
    free (url);
    
    return (ptr_server) ? 1 : 0;
}

/*
 * irc_server_outqueue_add: add a message in out queue
 */

void
irc_server_outqueue_add (struct t_irc_server *server, const char *command,
                         const char *msg1, const char *msg2, int modified)
{
    struct t_irc_outqueue *new_outqueue;

    new_outqueue = malloc (sizeof (*new_outqueue));
    if (new_outqueue)
    {
        new_outqueue->command = (command) ? strdup (command) : strdup ("unknown");
        new_outqueue->message_before_mod = (msg1) ? strdup (msg1) : NULL;
        new_outqueue->message_after_mod = (msg2) ? strdup (msg2) : NULL;
        new_outqueue->modified = modified;
        
        new_outqueue->prev_outqueue = server->last_outqueue;
        new_outqueue->next_outqueue = NULL;
        if (server->outqueue)
            server->last_outqueue->next_outqueue = new_outqueue;
        else
            server->outqueue = new_outqueue;
        server->last_outqueue = new_outqueue;
    }
}

/*
 * irc_server_outqueue_free: free a message in out queue
 */

void
irc_server_outqueue_free (struct t_irc_server *server,
                          struct t_irc_outqueue *outqueue)
{
    struct t_irc_outqueue *new_outqueue;
    
    /* remove outqueue message */
    if (server->last_outqueue == outqueue)
        server->last_outqueue = outqueue->prev_outqueue;
    if (outqueue->prev_outqueue)
    {
        (outqueue->prev_outqueue)->next_outqueue = outqueue->next_outqueue;
        new_outqueue = server->outqueue;
    }
    else
        new_outqueue = outqueue->next_outqueue;
    
    if (outqueue->next_outqueue)
        (outqueue->next_outqueue)->prev_outqueue = outqueue->prev_outqueue;

    /* free data */
    if (outqueue->command)
        free (outqueue->command);
    if (outqueue->message_before_mod)
        free (outqueue->message_before_mod);
    if (outqueue->message_after_mod)
        free (outqueue->message_after_mod);
    free (outqueue);
    server->outqueue = new_outqueue;
}

/*
 * irc_server_outqueue_free_all: free all outqueued messages
 */

void
irc_server_outqueue_free_all (struct t_irc_server *server)
{
    while (server->outqueue)
    {
        irc_server_outqueue_free (server, server->outqueue);
    }
}

/*
 * irc_server_free_data: free server data
 */

void
irc_server_free_data (struct t_irc_server *server)
{
    if (!server)
        return;
    
    /* free data */
    if (server->name)
        free (server->name);
    if (server->proxy)
        free (server->proxy);
    if (server->addresses)
        free (server->addresses);
    if (server->addresses_array)
        weechat_string_free_exploded (server->addresses_array);
    if (server->ports_array)
        free (server->ports_array);
    if (server->password)
        free (server->password);
    if (server->nicks)
        free (server->nicks);
    if (server->nicks_array)
        weechat_string_free_exploded (server->nicks_array);
    if (server->username)
        free (server->username);
    if (server->realname)
        free (server->realname);
    if (server->local_hostname)
        free (server->local_hostname);
    if (server->command)
        free (server->command);
    if (server->autojoin)
        free (server->autojoin);
    if (server->unterminated_message)
        free (server->unterminated_message);
    if (server->nick)
        free (server->nick);
    if (server->nick_modes)
        free (server->nick_modes);
    if (server->prefix)
        free (server->prefix);
    if (server->away_message)
        free (server->away_message);
    if (server->outqueue)
        irc_server_outqueue_free_all (server);
    if (server->channels)
        irc_channel_free_all (server);
    if (server->buffer_as_string)
        free (server->buffer_as_string);
}

/*
 * irc_server_free: free a server and remove it from servers queue
 */

void
irc_server_free (struct t_irc_server *server)
{
    struct t_irc_server *new_irc_servers;
    
    if (!server)
        return;
    
    /* close any opened channel/private */
    while (server->channels)
    {
        irc_channel_free (server, server->channels);
    }
    
    /* remove server from queue */
    if (last_irc_server == server)
        last_irc_server = server->prev_server;
    if (server->prev_server)
    {
        (server->prev_server)->next_server = server->next_server;
        new_irc_servers = irc_servers;
    }
    else
        new_irc_servers = server->next_server;
    
    if (server->next_server)
        (server->next_server)->prev_server = server->prev_server;
    
    irc_server_free_data (server);
    free (server);
    irc_servers = new_irc_servers;
}

/*
 * irc_server_free_all: free all allocated servers
 */

void
irc_server_free_all ()
{
    /* for each server in memory, remove it */
    while (irc_servers)
        irc_server_free (irc_servers);
}

/*
 * irc_server_new: creates a new server, and initialize it
 */

struct t_irc_server *
irc_server_new (const char *name, int autoconnect, int autoreconnect,
                int autoreconnect_delay, const char *proxy,
                const char *addresses, int ipv6, int ssl,
                const char *password, const char *nicks,
                const char *username, const char *realname,
                const char *local_hostname, const char *command,
                int command_delay, const char *autojoin, int autorejoin)
{
    struct t_irc_server *new_server;
    
    if (!name || !addresses)
        return NULL;
    
    if (weechat_irc_plugin->debug)
    {
        weechat_log_printf ("Creating new server (name:%s, proxy:%s, "
                            "addresses:%s, pwd:%s, nicks:%s, username:%s, "
                            "realname:%s, local_hostname: %s, command:%s, "
                            "autojoin:%s, autorejoin:%s)",
                            name, proxy, addresses, (password) ? password : "",
                            (nicks) ? nicks : "", (username) ? username : "",
                            (realname) ? realname : "",
                            (local_hostname) ? local_hostname : "",
                            (command) ? command : "",
                            (autojoin) ? autojoin : "",
                            (autorejoin) ? "on" : "off");
    }

    new_server = irc_server_alloc (name);
    if (new_server)
    {
        new_server->autoconnect = autoconnect;
        new_server->autoreconnect = autoreconnect;
        new_server->autoreconnect_delay = autoreconnect_delay;
        new_server->proxy = (proxy) ? strdup (proxy) : NULL;
        irc_server_set_addresses (new_server, addresses);
        new_server->ipv6 = ipv6;
        new_server->ssl = ssl;
        new_server->password = (password) ? strdup (password) : strdup ("");
        irc_server_set_nicks (new_server,
                              (nicks) ? nicks : IRC_SERVER_DEFAULT_NICKS);
        new_server->username =
            (username) ? strdup (username) : strdup ("weechat");
        new_server->realname =
            (realname) ? strdup (realname) : strdup ("realname");
        new_server->local_hostname =
            (local_hostname) ? strdup (local_hostname) : NULL;
        new_server->command =
            (command) ? strdup (command) : NULL;
        new_server->command_delay = command_delay;
        new_server->autojoin =
            (autojoin) ? strdup (autojoin) : NULL;
        new_server->autorejoin = autorejoin;
    }
    else
        return NULL;
    
    return new_server;
}

/*
 * irc_server_duplicate: duplicate a server
 *                       return: pointer to new server, NULL if error
 */

struct t_irc_server *
irc_server_duplicate (struct t_irc_server *server, const char *new_server_name)
{
    struct t_irc_server *new_server;
    int length, index_option;
    char *mask, *pos_option;
    const char *option_name;
    struct t_config_option *ptr_option;
    struct t_infolist *infolist;
    
    /* check if another server exists with this name */
    if (irc_server_search (new_server_name))
        return NULL;
    
    /* duplicate server */
    new_server = irc_server_new (new_server_name,
                                 server->autoconnect,
                                 server->autoreconnect,
                                 server->autoreconnect_delay,
                                 server->proxy,
                                 server->addresses,
                                 server->ipv6,
                                 server->ssl,
                                 server->password,
                                 server->nicks,
                                 server->username,
                                 server->realname,
                                 server->local_hostname,
                                 server->command,
                                 server->command_delay,
                                 server->autojoin,
                                 server->autorejoin);
    
    if (new_server)
    {
        /* duplicate options */
        length = 32 + strlen (server->name) + 1;
        mask = malloc (length);
        if (!mask)
            return 0;
        snprintf (mask, length, "irc.server.%s.*", server->name);
        infolist = weechat_infolist_get ("option", NULL, mask);
        free (mask);
        while (weechat_infolist_next (infolist))
        {
            weechat_config_search_with_string (weechat_infolist_string (infolist,
                                                                        "full_name"),
                                               NULL, NULL, &ptr_option,
                                               NULL);
            if (ptr_option)
            {
                option_name = weechat_infolist_string (infolist, "option_name");
                if (option_name)
                {
                    pos_option = strrchr (option_name, '.');
                    if (pos_option)
                    {
                        pos_option++;
                        
                        index_option = irc_config_search_server_option (pos_option);
                        if (index_option >= 0)
                        {
                            irc_server_new_option (new_server,
                                                   index_option,
                                                   weechat_infolist_string (infolist, "value"));
                        }
                    }
                }
            }
        }
        weechat_infolist_free (infolist);
    }
    
    return new_server;
}

/*
 * irc_server_rename: rename server (internal name)
 *                    return: 1 if ok, 0 if error
 */

int
irc_server_rename (struct t_irc_server *server, const char *new_server_name)
{
    int length;
    char *mask, *pos_option, *new_option_name, *buffer_name;
    const char *option_name;
    struct t_infolist *infolist;
    struct t_config_option *ptr_option;
    struct t_irc_channel *ptr_channel;
    
    /* check if another server exists with this name */
    if (irc_server_search (new_server_name))
        return 0;
    
    /* rename options */
    length = 32 + strlen (server->name) + 1;
    mask = malloc (length);
    if (!mask)
        return 0;
    snprintf (mask, length, "irc.server.%s.*", server->name);
    infolist = weechat_infolist_get ("option", NULL, mask);
    free (mask);
    while (weechat_infolist_next (infolist))
    {
        weechat_config_search_with_string (weechat_infolist_string (infolist,
                                                                    "full_name"),
                                           NULL, NULL, &ptr_option,
                                           NULL);
        if (ptr_option)
        {
            option_name = weechat_infolist_string (infolist, "option_name");
            if (option_name)
            {
                pos_option = strrchr (option_name, '.');
                if (pos_option)
                {
                    pos_option++;
                    length = strlen (new_server_name) + 1 + strlen (pos_option) + 1;
                    new_option_name = malloc (length);
                    if (new_option_name)
                    {
                        snprintf (new_option_name, length,
                                  "%s.%s", new_server_name, pos_option);
                        weechat_config_option_rename (ptr_option, new_option_name);
                        free (new_option_name);
                    }
                }
            }
        }
    }
    weechat_infolist_free (infolist);
    
    /* rename server */
    if (server->name)
        free (server->name);
    server->name = strdup (new_server_name);
    
    /* change name for buffers with this server */
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->buffer)
        {
            buffer_name = irc_buffer_build_name (server->name, ptr_channel->name);
            weechat_buffer_set (ptr_channel->buffer, "name", buffer_name);
        }
    }
    if (server->buffer)
    {
        buffer_name = irc_buffer_build_name (server->name, NULL);
        weechat_buffer_set (server->buffer, "name", buffer_name);
    }
    
    return 1;
}

/*
 * irc_server_send_signal: send a signal for an IRC message (received or sent)
 */

void
irc_server_send_signal (struct t_irc_server *server, const char *signal,
                        const char *command, const char *full_message)
{
    int length;
    char *str_signal;
    
    length = strlen (server->name) + 1 + strlen (signal) + 1 + strlen (command) + 1;
    str_signal = malloc (length);
    if (str_signal)
    {
        snprintf (str_signal, length,
                  "%s,%s_%s", server->name, signal, command);
        weechat_hook_signal_send (str_signal, WEECHAT_HOOK_SIGNAL_STRING,
                                  (void *)full_message);
        free (str_signal);
    }
}

/*
 * irc_server_send: send data to IRC server
 *                  return number of bytes sent, -1 if error
 */

int
irc_server_send (struct t_irc_server *server, const char *buffer, int size_buf)
{
    int rc;
    
    if (!server)
    {
        weechat_printf (NULL,
                        _("%s%s: error sending data to IRC server: null "
                          "pointer (please report problem to developers)"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return 0;
    }

    if (size_buf <= 0)
    {
        weechat_printf (server->buffer,
                        _("%s%s: error sending data to IRC server: empty "
                          "buffer (please report problem to "
                          "developers)"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
        return 0;
    }
    
#ifdef HAVE_GNUTLS
    if (server->ssl_connected)
        rc = gnutls_record_send (server->gnutls_sess, buffer, size_buf);
    else
#endif
        rc = send (server->sock, buffer, size_buf, 0);
    
    if (rc < 0)
    {
        weechat_printf (server->buffer,
                        _("%s%s: error sending data to IRC server (%s)"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME, strerror (errno));
    }
    
    return rc;
}

/*
 * irc_server_outqueue_send: send a message from outqueue
 */

void
irc_server_outqueue_send (struct t_irc_server *server)
{
    time_t time_now;
    char *pos;
    
    if (server->outqueue)
    {
        time_now = time (NULL);
        
        /* detect if system clock has been changed (now lower than before) */
        if (server->last_user_message > time_now)
            server->last_user_message = time_now;
        
        if (time_now >= server->last_user_message +
            weechat_config_integer (irc_config_network_anti_flood))
        {
            if (server->outqueue->message_before_mod)
            {
                pos = strchr (server->outqueue->message_before_mod, '\r');
                if (pos)
                    pos[0] = '\0';
                irc_debug_printf (server, 1, 0,
                                  server->outqueue->message_before_mod);
                if (pos)
                    pos[0] = '\r';
            }
            if (server->outqueue->message_after_mod)
            {
                pos = strchr (server->outqueue->message_after_mod, '\r');
                if (pos)
                    pos[0] = '\0';
                irc_debug_printf (server, 1, server->outqueue->modified,
                                  server->outqueue->message_after_mod);
                if (pos)
                    pos[0] = '\r';
                
                /* send signal with command that will be sent to server */
                irc_server_send_signal (server, "irc_out",
                                        server->outqueue->command,
                                        server->outqueue->message_after_mod);
                
                /* send command */
                irc_server_send (server, server->outqueue->message_after_mod,
                                 strlen (server->outqueue->message_after_mod));
                server->last_user_message = time_now;
            }
            irc_server_outqueue_free (server, server->outqueue);
        }
    }
}

/*
 * irc_server_parse_message: parse IRC message and return pointer to
 *                           host, command, channel, target nick and arguments
 *                           (if any)
 */

void
irc_server_parse_message (const char *message, char **nick, char **host,
                          char **command, char **channel, char **arguments)
{
    const char *pos, *pos2, *pos3, *pos4;

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
    
    if (message[0] == ':')
    {
        pos2 = strchr (message, '!');
        pos = strchr (message, ' ');
        if (pos2)
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
    
    if (pos && pos[0])
    {
        while (pos[0] == ' ')
        {
            pos++;
        }
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
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
                        if (nick)
                        {
                            if (pos3)
                                *nick = weechat_strndup (pos2, pos3 - pos2);
                            else
                                *nick = strdup (pos2);
                        }
                    }
                    if (pos3)
                    {
                        pos3++;
                        while (pos3[0] == ' ')
                        {
                            pos3++;
                        }
                        if (irc_channel_is_channel (pos3))
                        {
                            pos4 = strchr (pos3, ' ');
                            if (channel)
                            {
                                if (pos4)
                                    *channel = weechat_strndup (pos3, pos4 - pos3);
                                else
                                    *channel = strdup (pos3);
                            }
                        }
                    }
                }
            }
        }
    }
}

/*
 * irc_server_send_one_msg: send one message to IRC server
 *                          return: 1 if ok, 0 if error
 */

int
irc_server_send_one_msg (struct t_irc_server *server, const char *message)
{
    static char buffer[4096];
    const char *ptr_msg;
    char *new_msg, *pos, *nick, *command, *channel;
    char *ptr_chan_nick, *msg_encoded;
    char str_modifier[64], modifier_data[256];
    int rc, queue, first_message;
    time_t time_now;
    
    rc = 1;
    
    irc_server_parse_message (message, &nick, NULL, &command, &channel, NULL);
    snprintf (str_modifier, sizeof (str_modifier),
              "irc_out_%s",
              (command) ? command : "unknown");
    new_msg = weechat_hook_modifier_exec (str_modifier,
                                          server->name,
                                          message);
    
    /* no changes in new message */
    if (new_msg && (strcmp (message, new_msg) == 0))
    {
        free (new_msg);
        new_msg = NULL;
    }
    
    /* message not dropped? */
    if (!new_msg || new_msg[0])
    {
        first_message = 1;
        ptr_msg = (new_msg) ? new_msg : message;

        msg_encoded = NULL;
        ptr_chan_nick = (channel) ? channel : nick;
        if (ptr_chan_nick)
        {
            snprintf (modifier_data, sizeof (modifier_data),
                      "%s.%s.%s",
                      weechat_plugin->name,
                      server->name,
                      ptr_chan_nick);
        }
        else
        {
            snprintf (modifier_data, sizeof (modifier_data),
                      "%s.%s.%s",
                      weechat_plugin->name,
                      server->name,
                      ptr_chan_nick);
        }
        msg_encoded = weechat_hook_modifier_exec ("charset_encode",
                                                  modifier_data,
                                                  ptr_msg);
        
        if (msg_encoded)
            ptr_msg = msg_encoded;
        
        while (rc && ptr_msg && ptr_msg[0])
        {
            pos = strchr (ptr_msg, '\n');
            if (pos)
                pos[0] = '\0';
            
            snprintf (buffer, sizeof (buffer) - 1, "%s\r\n", ptr_msg);

            /* anti-flood: look whether we should queue outgoing message or not */
            time_now = time (NULL);
            
            /* detect if system clock has been changed (now lower than before) */
            if (server->last_user_message > time_now)
                server->last_user_message = time_now;
            
            queue = 0;
            if ((server->queue_msg)
                && ((server->outqueue)
                    || ((weechat_config_integer (irc_config_network_anti_flood) > 0)
                        && (time_now - server->last_user_message <
                            weechat_config_integer (irc_config_network_anti_flood)))))
                queue = 1;
            
            /* if queue, then only queue message and send nothing now */
            if (queue)
            {
                irc_server_outqueue_add (server, command,
                                         (new_msg && first_message) ? message : NULL,
                                         buffer,
                                         (new_msg) ? 1 : 0);
            }
            else
            {
                if (first_message)
                    irc_debug_printf (server, 1, 0, message);
                if (new_msg)
                    irc_debug_printf (server, 1, 1, ptr_msg);
                
                /* send signal with command that will be sent to server */
                irc_server_send_signal (server, "irc_out",
                                        (command) ? command : "unknown",
                                        ptr_msg);
                
                if (irc_server_send (server, buffer, strlen (buffer)) <= 0)
                    rc = 0;
                else
                {
                    if (server->queue_msg)
                        server->last_user_message = time_now;
                }
            }
            if (pos)
            {
                pos[0] = '\n';
                ptr_msg = pos + 1;
            }
            else
                ptr_msg = NULL;
            
            first_message = 0;
        }
        if (msg_encoded)
            free (msg_encoded);
    }
    else
        irc_debug_printf (server, 1, 1, _("(message dropped)"));
    
    if (nick)
        free (nick);
    if (command)
        free (command);
    if (channel)
        free (channel);
    if (new_msg)
        free (new_msg);
    
    return rc;
}

/*
 * irc_server_sendf: send formatted data to IRC server
 *                   many messages may be sent, separated by '\n'
 */

void
irc_server_sendf (struct t_irc_server *server, const char *format, ...)
{
    va_list args;
    static char buffer[4096];
    char **items;
    int i, items_count;
    
    if (!server)
        return;
    
    va_start (args, format);
    vsnprintf (buffer, sizeof (buffer) - 1, format, args);
    va_end (args);    

    items = weechat_string_explode (buffer, "\n", 0, 0, &items_count);
    for (i = 0; i < items_count; i++)
    {
        if (!irc_server_send_one_msg (server, items[i]))
            break;
    }
    if (items)
        weechat_string_free_exploded (items);
}

/*
 * irc_server_msgq_add_msg: add a message to received messages queue (at the end)
 */

void
irc_server_msgq_add_msg (struct t_irc_server *server, const char *msg)
{
    struct t_irc_message *message;
    
    if (!server->unterminated_message && !msg[0])
        return;
    
    message = malloc (sizeof (*message));
    if (!message)
    {
        weechat_printf (server->buffer,
                        _("%s%s: not enough memory for received IRC "
                          "message"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
        return;
    }
    message->server = server;
    if (server->unterminated_message)
    {
        message->data = malloc (strlen (server->unterminated_message) +
                                strlen (msg) + 1);
        if (!message->data)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for received IRC "
                              "message"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
        }
        else
        {
            strcpy (message->data, server->unterminated_message);
            strcat (message->data, msg);
        }
        free (server->unterminated_message);
        server->unterminated_message = NULL;
    }
    else
        message->data = strdup (msg);
    message->next_message = NULL;
    
    if (irc_msgq_last_msg)
    {
        irc_msgq_last_msg->next_message = message;
        irc_msgq_last_msg = message;
    }
    else
    {
        irc_recv_msgq = message;
        irc_msgq_last_msg = message;
    }
}

/*
 * irc_server_msgq_add_unterminated: add an unterminated message to queue
 */

void
irc_server_msgq_add_unterminated (struct t_irc_server *server, const char *string)
{
    if (!string[0])
        return;
    
    if (server->unterminated_message)
    {
        server->unterminated_message =
            realloc (server->unterminated_message,
                     (strlen (server->unterminated_message) +
                      strlen (string) + 1));
        if (!server->unterminated_message)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for received IRC "
                              "message"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
        }
        else
            strcat (server->unterminated_message, string);
    }
    else
    {
        server->unterminated_message = strdup (string);
        if (!server->unterminated_message)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory for received IRC "
                              "message"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
        }
    }
}

/*
 * irc_server_msgq_add_buffer: explode received buffer, creating queued messages
 */

void
irc_server_msgq_add_buffer (struct t_irc_server *server, const char *buffer)
{
    char *pos_cr, *pos_lf;

    while (buffer[0])
    {
        pos_cr = strchr (buffer, '\r');
        pos_lf = strchr (buffer, '\n');
        
        if (!pos_cr && !pos_lf)
        {
            /* no CR/LF found => add to unterminated and return */
            irc_server_msgq_add_unterminated (server, buffer);
            return;
        }
        
        if (pos_cr && ((!pos_lf) || (pos_lf > pos_cr)))
        {
            /* found '\r' first => ignore this char */
            pos_cr[0] = '\0';
            irc_server_msgq_add_unterminated (server, buffer);
            buffer = pos_cr + 1;
        }
        else
        {
            /* found: '\n' first => terminate message */
            pos_lf[0] = '\0';
            irc_server_msgq_add_msg (server, buffer);
            buffer = pos_lf + 1;
        }
    }
}

/*
 * irc_server_msgq_flush: flush message queue
 */

void
irc_server_msgq_flush ()
{
    struct t_irc_message *next;
    char *ptr_data, *new_msg, *ptr_msg, *pos;
    char *nick, *host, *command, *channel, *arguments, *msg_decoded;
    char str_modifier[64], modifier_data[256], *ptr_chan_nick;
    
    while (irc_recv_msgq)
    {
        if (irc_recv_msgq->data)
        {
            ptr_data = irc_recv_msgq->data;
            while (ptr_data[0] == ' ')
            {
                ptr_data++;
            }
            
            if (ptr_data[0])
            {
                irc_debug_printf (irc_recv_msgq->server, 0, 0, ptr_data);
                
                irc_server_parse_message (ptr_data, NULL, NULL, &command,
                                          NULL, NULL);
                snprintf (str_modifier, sizeof (str_modifier),
                          "irc_in_%s",
                          (command) ? command : "unknown");
                new_msg = weechat_hook_modifier_exec (str_modifier,
                                                      irc_recv_msgq->server->name,
                                                      ptr_data);
                if (command)
                    free (command);
                
                /* no changes in new message */
                if (new_msg && (strcmp (ptr_data, new_msg) == 0))
                {
                    free (new_msg);
                    new_msg = NULL;
                }
                
                /* message not dropped? */
                if (!new_msg || new_msg[0])
                {
                    /* use new message (returned by plugin) */
                    ptr_msg = (new_msg) ? new_msg : ptr_data;
                    
                    while (ptr_msg && ptr_msg[0])
                    {
                        pos = strchr (ptr_msg, '\n');
                        if (pos)
                            pos[0] = '\0';
                        
                        if (new_msg)
                            irc_debug_printf (irc_recv_msgq->server, 0, 1,
                                              ptr_msg);
                        
                        irc_server_parse_message (ptr_msg, &nick, &host,
                                                  &command, &channel,
                                                  &arguments);
                        
                        /* convert charset for message */
                        ptr_chan_nick = (channel) ? channel : nick;
                        if (ptr_chan_nick)
                        {
                            snprintf (modifier_data, sizeof (modifier_data),
                                      "%s.%s.%s",
                                      weechat_plugin->name,
                                      irc_recv_msgq->server->name,
                                      ptr_chan_nick);
                        }
                        else
                        {
                            snprintf (modifier_data, sizeof (modifier_data),
                                      "%s.%s",
                                      weechat_plugin->name,
                                      irc_recv_msgq->server->name);
                        }
                        msg_decoded = weechat_hook_modifier_exec ("charset_decode",
                                                                  modifier_data,
                                                                  ptr_msg);
                        
                        /* parse and execute command */
                        irc_protocol_recv_command (irc_recv_msgq->server,
                                                   (msg_decoded) ? msg_decoded : ptr_msg,
                                                   host,
                                                   command,
                                                   arguments);
                        
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
                        if (msg_decoded)
                            free (msg_decoded);
                        
                        if (pos)
                        {
                            pos[0] = '\n';
                            ptr_msg = pos + 1;
                        }
                        else
                            ptr_msg = NULL;
                    }
                }
                else
                {
                    irc_debug_printf (irc_recv_msgq->server, 0, 1,
                                      _("(message dropped)"));
                }
                if (new_msg)
                    free (new_msg);
            }
            free (irc_recv_msgq->data);
        }
        
        next = irc_recv_msgq->next_message;
        free (irc_recv_msgq);
        irc_recv_msgq = next;
        if (!irc_recv_msgq)
            irc_msgq_last_msg = NULL;
    }
}

/*
 * irc_server_recv_cb: receive data from an irc server
 */

int
irc_server_recv_cb (void *arg_server)
{
    struct t_irc_server *server;
    static char buffer[4096 + 2];
    int num_read;
    
    server = (struct t_irc_server *)arg_server;
    
    if (!server)
        return WEECHAT_RC_ERROR;
    
#ifdef HAVE_GNUTLS
    if (server->ssl_connected)
        num_read = gnutls_record_recv (server->gnutls_sess, buffer,
                                       sizeof (buffer) - 2);
    else
#endif
        num_read = recv (server->sock, buffer, sizeof (buffer) - 2, 0);
    
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        irc_server_msgq_add_buffer (server, buffer);
        irc_server_msgq_flush ();
    }
    else
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot read data from socket, "
                          "disconnecting from server..."),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
        irc_server_disconnect (server, 1);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_timer_cb: timer called each second to perform some operations
 *                      on servers
 */

int
irc_server_timer_cb (void *data)
{
    struct t_irc_server *ptr_server;
    time_t new_time;
    static struct timeval tv;
    int diff;
    
    /* make C compiler happy */
    (void) data;
    
    new_time = time (NULL);
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* check if reconnection is pending */
        if ((!ptr_server->is_connected)
            && (ptr_server->reconnect_start > 0)
            && (new_time >= (ptr_server->reconnect_start + ptr_server->autoreconnect_delay)))
            irc_server_reconnect (ptr_server);
        else
        {
            if (ptr_server->is_connected)
            {
                /* send queued messages */
                irc_server_outqueue_send (ptr_server);
                
                /* check for lag */
                if ((ptr_server->lag_check_time.tv_sec == 0)
                    && (new_time >= ptr_server->lag_next_check))
                {
                    irc_server_sendf (ptr_server, "PING %s",
                                      ptr_server->addresses_array[ptr_server->current_address]);
                    gettimeofday (&(ptr_server->lag_check_time), NULL);
                }
                
                /* check if it's time to autojoin channels (after command delay) */
                if ((ptr_server->command_time != 0)
                    && (new_time >= ptr_server->command_time + ptr_server->command_delay))
                {
                    irc_server_autojoin_channels (ptr_server);
                    ptr_server->command_time = 0;
                }
                
                /* lag timeout => disconnect */
                if ((ptr_server->lag_check_time.tv_sec != 0)
                    && (weechat_config_integer (irc_config_network_lag_disconnect) > 0))
                {
                    gettimeofday (&tv, NULL);
                    diff = (int) weechat_timeval_diff (&(ptr_server->lag_check_time),
                                                       &tv);
                    if (diff / 1000 > weechat_config_integer (irc_config_network_lag_disconnect) * 60)
                    {
                        weechat_printf (ptr_server->buffer,
                                        _("%s%s: lag is high, "
                                          "disconnecting from "
                                          "server..."),
                                        irc_buffer_get_server_prefix (ptr_server,
                                                                      NULL),
                                        IRC_PLUGIN_NAME);
                        irc_server_disconnect (ptr_server, 1);
                    }
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_timer_check_away_cb: timer called to check away on servers
 *                                 (according to option "irc_check_away")
 */

int
irc_server_timer_check_away_cb (void *data)
{
    /* make C compiler happy */
    (void) data;
    
    if (weechat_config_integer (irc_config_network_away_check) > 0)
        irc_server_check_away ();
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_close_connection: close server connection
 */

void
irc_server_close_connection (struct t_irc_server *server)
{
    if (server->hook_fd)
    {
        weechat_unhook (server->hook_fd);
        server->hook_fd = NULL;
    }
    
    if (server->hook_connect)
    {
        weechat_unhook (server->hook_connect);
        server->hook_connect = NULL;
    }
    else
    {
#ifdef HAVE_GNUTLS
        /* close SSL connection */
        if ((server->sock != -1) && (server->ssl_connected))
        {
            if (server->ssl_connected)
                gnutls_bye (server->gnutls_sess, GNUTLS_SHUT_WR);
            if (server->ssl_connected)
                gnutls_deinit (server->gnutls_sess);
        }
#endif
    }
    if (server->sock != -1)
    {
        close (server->sock);
        server->sock = -1;
    }
    
    /* free any pending message */
    if (server->unterminated_message)
    {
        free (server->unterminated_message);
        server->unterminated_message = NULL;
    }
    irc_server_outqueue_free_all (server);
    
    /* server is now disconnected */
    server->is_connected = 0;
    server->ssl_connected = 0;
}

/*
 * irc_server_reconnect_schedule: schedule reconnect for a server
 */

void
irc_server_reconnect_schedule (struct t_irc_server *server)
{
    server->current_address = 0;
    if (server->autoreconnect)
    {
        server->reconnect_start = time (NULL);
        weechat_printf (server->buffer,
                        _("%s%s: reconnecting to server in %d %s"),
                        irc_buffer_get_server_prefix (server, NULL),
                        IRC_PLUGIN_NAME,
                        server->autoreconnect_delay,
                        NG_("second", "seconds",
                            server->autoreconnect_delay));
    }
    else
        server->reconnect_start = 0;
}

/*
 * irc_server_login: login to IRC server
 */

void
irc_server_login (struct t_irc_server *server)
{
    if ((server->password) && (server->password[0]))
        irc_server_sendf (server, "PASS %s", server->password);
    
    if (!server->nick)
        irc_server_set_nick (server,
                             (server->nicks_array) ?
                             server->nicks_array[0] : "weechat");
    
    irc_server_sendf (server,
                      "NICK %s\n"
                      "USER %s %s %s :%s",
                      server->nick,
                      (server->username && server->username[0]) ?
                      server->username : "weechat",
                      (server->username && server->username[0]) ?
                      server->username : "weechat",
                      server->addresses_array[server->current_address],
                      (server->realname && server->realname[0]) ?
                      server->realname : "weechat");
}

/*
 * irc_server_switch_address: switch address and try another
 *                            (called if connection failed with an address/port)
 */

void
irc_server_switch_address (struct t_irc_server *server)
{
    if ((server->addresses_count > 1)
        && (server->current_address < server->addresses_count - 1))
    {
        server->current_address++;
        weechat_printf (server->buffer,
                        _("%s%s: switching address to %s/%d"),
                        irc_buffer_get_server_prefix (server, NULL),
                        IRC_PLUGIN_NAME,
                        server->addresses_array[server->current_address],
                        server->ports_array[server->current_address]);
        irc_server_connect (server, 0);
    }
    else
        irc_server_reconnect_schedule (server);
}

/*
 * irc_server_connect_cb: read connection status
 */

int
irc_server_connect_cb (void *arg_server, int status, const char *ip_address)
{
    struct t_irc_server *server;
    
    server = (struct t_irc_server *)arg_server;
    
    server->hook_connect = NULL;
    
    switch (status)
    {
        case WEECHAT_HOOK_CONNECT_OK:
            /* login to server */
            weechat_printf (server->buffer,
                            _("%s%s: connected to %s (%s)"),
                            irc_buffer_get_server_prefix (server, NULL),
                            IRC_PLUGIN_NAME,
                            server->addresses_array[server->current_address],
                            (ip_address) ? ip_address : "?");
            irc_server_login (server);
            server->hook_fd = weechat_hook_fd (server->sock,
                                               1, 0, 0,
                                               &irc_server_recv_cb,
                                               server);
            break;
        case WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND:
            weechat_printf (server->buffer,
                            (server->proxy && server->proxy[0]) ?
                            _("%s%s: proxy address \"%s\" not found") :
                            _("%s%s: address \"%s\" not found"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME,
                            server->addresses_array[server->current_address]);
            irc_server_close_connection (server);
            irc_server_switch_address (server);
            break;
        case WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND:
            weechat_printf (server->buffer,
                            (server->proxy && server->proxy[0]) ?
                            _("%s%s: proxy IP address not found") :
                            _("%s%s: IP address not found"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_switch_address (server);
            break;
        case WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED:
            weechat_printf (server->buffer,
                            (server->proxy && server->proxy[0]) ?
                            _("%s%s: proxy connection refused") :
                            _("%s%s: connection refused"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_switch_address (server);
            break;
        case WEECHAT_HOOK_CONNECT_PROXY_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: proxy fails to establish "
                              "connection to server "
                              "(check username/password if used "
                              "and if IRC server address/port is "
                              "allowed by proxy)"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_switch_address (server);
            break;
        case WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: unable to set local hostname/IP"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: GnuTLS init error"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: GnuTLS handshake failed"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_switch_address (server);
            break;
        case WEECHAT_HOOK_CONNECT_MEMORY_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            irc_server_close_connection (server);
            irc_server_reconnect_schedule (server);
            break;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_create_buffer: create a buffer for an IRC server
 */

struct t_gui_buffer *
irc_server_create_buffer (struct t_irc_server *server, int all_servers)
{
    char buffer_name[256];
    
    if (all_servers)
    {
        snprintf (buffer_name, sizeof (buffer_name),
                  IRC_BUFFER_ALL_SERVERS_NAME);
    }
    else
    {
        snprintf (buffer_name, sizeof (buffer_name),
                  "server.%s", server->name);
    }
    server->buffer = weechat_buffer_new (buffer_name,
                                         NULL, NULL,
                                         &irc_buffer_close_cb, NULL);
    if (!server->buffer)
        return NULL;
    
    weechat_buffer_set (server->buffer, "short_name",
                        (weechat_config_boolean (irc_config_look_one_server_buffer)) ?
                        IRC_BUFFER_ALL_SERVERS_NAME : server->name);
    weechat_buffer_set (server->buffer, "localvar_set_server",
                        (weechat_config_boolean (irc_config_look_one_server_buffer)) ?
                        IRC_BUFFER_ALL_SERVERS_NAME : server->name);
    weechat_buffer_set (server->buffer, "localvar_set_channel",
                        (weechat_config_boolean (irc_config_look_one_server_buffer)) ?
                        IRC_BUFFER_ALL_SERVERS_NAME : server->name);
    
    weechat_hook_signal_send ("logger_backlog",
                              WEECHAT_HOOK_SIGNAL_POINTER, server->buffer);
    
    /* set highlights settings on server buffer */
    if (server->nick)
        weechat_buffer_set (server->buffer, "highlight_words", server->nick);
    if (weechat_config_string (irc_config_look_highlight_tags)
        && weechat_config_string (irc_config_look_highlight_tags)[0])
    {
        weechat_buffer_set (server->buffer, "highlight_tags",
                            weechat_config_string (irc_config_look_highlight_tags));
    }
    
    return server->buffer;
}

/*
 * irc_server_connect: connect to an IRC server
 *                     Return: 1 if ok
 *                             0 if error
 */

int
irc_server_connect (struct t_irc_server *server, int disable_autojoin)
{
    int set, length;
    char *option_name;
    struct t_config_option *proxy_type, *proxy_ipv6, *proxy_address, *proxy_port;
    const char *str_proxy_type, *str_proxy_address;
    
    proxy_type = NULL;
    proxy_ipv6 = NULL;
    proxy_address = NULL;
    proxy_port = NULL;
    str_proxy_type = NULL;
    str_proxy_address = NULL;
    
    if (!server->buffer)
    {
        if (weechat_config_boolean (irc_config_look_one_server_buffer)
            && irc_buffer_servers)
        {
            server->buffer = irc_buffer_servers;
        }
        else
        {
            if (!irc_server_create_buffer (server,
                                           weechat_config_boolean (irc_config_look_one_server_buffer)))
                return 0;
        }
        
        if (weechat_config_boolean (irc_config_look_one_server_buffer))
        {
            irc_current_server = server;
            if (!irc_buffer_servers)
                irc_buffer_servers = server->buffer;
        }
        
        weechat_buffer_set (server->buffer, "display", "1");
        
        weechat_bar_item_update ("buffer_name");
        
        weechat_buffer_set (server->buffer, "key_bind_meta-s",
                            "/command irc /server switch");
    }
    
    if (server->proxy && server->proxy[0])
    {
        length = 32 + strlen (server->proxy) + 1;
        option_name = malloc (length);
        if (!option_name)
        {
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME);
            return 0;
        }
        snprintf (option_name, length, "weechat.proxy.%s.type",
                  server->proxy);
        proxy_type = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.ipv6",
                  server->proxy);
        proxy_ipv6 = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.address",
                  server->proxy);
        proxy_address = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.port",
                  server->proxy);
        proxy_port = weechat_config_get (option_name);
        free (option_name);
        if (!proxy_type || !proxy_address)
        {
            weechat_printf (server->buffer,
                            _("%s%s: proxy \"%s\" not found for server "
                              "\"%s\", cannot connect"),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME, server->proxy, server->name);
            return 0;
        }
        str_proxy_type = weechat_config_string (proxy_type);
        str_proxy_address = weechat_config_string (proxy_address);
        if (!str_proxy_type[0] || !proxy_ipv6 || !str_proxy_address[0]
            || !proxy_port)
        {
            weechat_printf (server->buffer,
                            _("%s%s: missing proxy settings, check options "
                              "for proxy \"%s\""),
                            irc_buffer_get_server_prefix (server, "error"),
                            IRC_PLUGIN_NAME, server->proxy);
            return 0;
        }
    }
    
    if (!server->addresses || !server->addresses[0])
    {
        weechat_printf (server->buffer,
                        _("%s%s: addresses not defined for server \"%s\", "
                          "cannot connect"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME, server->name);
        return 0;
    }
    
    if (!server->nicks || !server->nicks[0])
    {
        weechat_printf (server->buffer,
                        _("%s%s: nicks not defined for server \"%s\", "
                          "cannot connect"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME, server->name);
        return 0;
    }
    
#ifndef HAVE_GNUTLS
    if (server->ssl)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot connect with SSL since WeeChat "
                          "was not built with GnuTLS support"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
        return 0;
    }
#endif
    if (proxy_type)
    {
        weechat_printf (server->buffer,
                        _("%s%s: connecting to server %s/%d%s%s via %s "
                          "proxy %s/%d%s..."),
                        irc_buffer_get_server_prefix (server, NULL),
                        IRC_PLUGIN_NAME,
                        server->addresses_array[server->current_address],
                        server->ports_array[server->current_address],
                        (server->ipv6) ? " (IPv6)" : "",
                        (server->ssl) ? " (SSL)" : "",
                        str_proxy_type,
                        str_proxy_address,
                        weechat_config_integer (proxy_port),
                        (weechat_config_boolean (proxy_ipv6)) ? " (IPv6)" : "");
        weechat_log_printf (_("Connecting to server %s/%d%s%s via %s proxy "
                              "%s/%d%s..."),
                            server->addresses_array[server->current_address],
                            server->ports_array[server->current_address],
                            (server->ipv6) ? " (IPv6)" : "",
                            (server->ssl) ? " (SSL)" : "",
                            str_proxy_type,
                            str_proxy_address,
                            weechat_config_integer (proxy_port),
                            (weechat_config_boolean (proxy_ipv6)) ? " (IPv6)" : "");
    }
    else
    {
        weechat_printf (server->buffer,
                        _("%s%s: connecting to server %s/%d%s%s..."),
                        irc_buffer_get_server_prefix (server, NULL),
                        IRC_PLUGIN_NAME,
                        server->addresses_array[server->current_address],
                        server->ports_array[server->current_address],
                        (server->ipv6) ? " (IPv6)" : "",
                        (server->ssl) ? " (SSL)" : "");
        weechat_log_printf (_("%s%s: connecting to server %s/%d%s%s..."),
                            "",
                            IRC_PLUGIN_NAME,
                            server->addresses_array[server->current_address],
                            server->ports_array[server->current_address],
                            (server->ipv6) ? " (IPv6)" : "",
                            (server->ssl) ? " (SSL)" : "");
    }
    
    /* close connection if open */
    irc_server_close_connection (server);
    
    /* create socket and set options */
    if (proxy_type)
    {
        server->sock = socket ((weechat_config_integer (proxy_ipv6)) ?
                               AF_INET6 : AF_INET,
                               SOCK_STREAM, 0);
    }
    else
    {
        server->sock = socket ((server->ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    }
    if (server->sock == -1)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot create socket"),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
        return 0;
    }
    
    /* set SO_REUSEADDR option for socket */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_REUSEADDR,
        (void *) &set, sizeof (set)) == -1)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot set socket option "
                          "\"SO_REUSEADDR\""),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
    }
    
    /* set SO_KEEPALIVE option for socket */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_KEEPALIVE,
        (void *) &set, sizeof (set)) == -1)
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot set socket option "
                          "\"SO_KEEPALIVE\""),
                        irc_buffer_get_server_prefix (server, "error"),
                        IRC_PLUGIN_NAME);
    }
    
    /* init SSL if asked */
    server->ssl_connected = 0;
#ifdef HAVE_GNUTLS
    if (server->ssl)
        server->ssl_connected = 1;
#endif
    
    server->disable_autojoin = disable_autojoin;
    
    server->hook_connect = weechat_hook_connect (server->proxy,
                                                 server->addresses_array[server->current_address],
                                                 server->ports_array[server->current_address],
                                                 server->sock,
                                                 server->ipv6,
#ifdef HAVE_GNUTLS
                                                 (server->ssl_connected) ? &server->gnutls_sess : NULL,
#else
                                                 NULL,
#endif
                                                 server->local_hostname,
                                                 irc_server_connect_cb,
                                                 server);
    
    /* send signal "irc_server_connecting" with server name */
    weechat_hook_signal_send ("irc_server_connecting",
                              WEECHAT_HOOK_SIGNAL_STRING, server->name);
    
    return 1;
}

/*
 * irc_server_reconnect: reconnect to a server (after disconnection)
 */

void
irc_server_reconnect (struct t_irc_server *server)
{
    weechat_printf (server->buffer,
                    _("%s%s: reconnecting to server..."),
                    irc_buffer_get_server_prefix (server, NULL),
                    IRC_PLUGIN_NAME);
    server->reconnect_start = 0;
    server->current_address = 0;
    
    if (irc_server_connect (server, 0))
        server->reconnect_join = 1;
    else
        irc_server_reconnect_schedule (server);
}

/*
 * irc_server_auto_connect: auto-connect to servers (called at startup)
 */

void
irc_server_auto_connect (int auto_connect)
{
    struct t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if ((auto_connect) && (ptr_server->autoconnect))
        {
            if (!irc_server_connect (ptr_server, 0))
                irc_server_reconnect_schedule (ptr_server);
        }
    }
}

/*
 * irc_server_disconnect: disconnect from an irc server
 */

void
irc_server_disconnect (struct t_irc_server *server, int reconnect)
{
    struct t_irc_channel *ptr_channel;
    
    if (server->is_connected)
    {
        /* write disconnection message on each channel/private buffer */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_nick_free_all (ptr_channel);
            weechat_printf (ptr_channel->buffer,
                            _("%s%s: disconnected from server"),
                            "",
                            IRC_PLUGIN_NAME);
        }
    }
    
    irc_server_close_connection (server);
    
    if (server->buffer)
        weechat_printf (server->buffer,
                        _("%s%s: disconnected from server"),
                        irc_buffer_get_server_prefix (server, NULL),
                        IRC_PLUGIN_NAME);
    
    server->current_address = 0;
    if (server->nick_modes)
    {
        free (server->nick_modes);
        server->nick_modes = NULL;
        weechat_bar_item_update ("input_prompt");
    }
    if (server->prefix)
    {
        free (server->prefix);
        server->prefix = NULL;
    }
    server->is_away = 0;
    server->away_time = 0;
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) +
        weechat_config_integer (irc_config_network_lag_check);
    
    if ((reconnect) && (server->autoreconnect))
        irc_server_reconnect_schedule (server);
    else
        server->reconnect_start = 0;
    
    /* discard current nick if no reconnection asked */
    if (!reconnect && server->nick)
        irc_server_set_nick (server, NULL);

    /* send signal "irc_server_disconnected" with server name */
    weechat_hook_signal_send ("irc_server_disconnected",
                              WEECHAT_HOOK_SIGNAL_STRING, server->name);
}

/*
 * irc_server_disconnect_all: disconnect from all irc servers
 */

void
irc_server_disconnect_all ()
{
    struct t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        irc_server_disconnect (ptr_server, 0);
    }
}

/*
 * irc_server_autojoin_channels: autojoin (or rejoin) channels
 */

void
irc_server_autojoin_channels (struct t_irc_server *server)
{
    struct t_irc_channel *ptr_channel;
    
    /* auto-join after disconnection (only rejoins opened channels) */
    if (!server->disable_autojoin && server->reconnect_join && server->channels)
    {
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            {
                if (ptr_channel->key)
                    irc_server_sendf (server, "JOIN %s %s",
                                      ptr_channel->name, ptr_channel->key);
                else
                    irc_server_sendf (server, "JOIN %s",
                                      ptr_channel->name);
            }
        }
        server->reconnect_join = 0;
    }
    else
    {
        /* auto-join when connecting to server for first time */
        if (!server->disable_autojoin && server->autojoin && server->autojoin[0])
            irc_command_join_server (server, server->autojoin);
    }

    server->disable_autojoin = 0;
}

/*
 * irc_server_search: return pointer on a server with a name
 */

struct t_irc_server *
irc_server_search (const char *server_name)
{
    struct t_irc_server *ptr_server;
    
    if (!server_name)
        return NULL;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (strcmp (ptr_server->name, server_name) == 0)
            return ptr_server;
    }
    return NULL;
}

/*
 * irc_server_get_number_connected: returns number of connected server
 */

int
irc_server_get_number_connected ()
{
    struct t_irc_server *ptr_server;
    int number;
    
    number = 0;
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
            number++;
    }
    return number;
}

/*
 * irc_server_get_number_buffer: returns position of a server and total number of
 *                               buffers with a buffer
 */

void
irc_server_get_number_buffer (struct t_irc_server *server,
                              int *server_pos, int *server_total)
{
    struct t_irc_server *ptr_server;
    
    *server_pos = 0;
    *server_total = 0;
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->buffer)
        {
            (*server_total)++;
            if (ptr_server == server)
                *server_pos = *server_total;
        }
    }
}

/*
 * irc_server_get_channel_count: return number of channels for server
 */

int
irc_server_get_channel_count (struct t_irc_server *server)
{
    int count;
    struct t_irc_channel *ptr_channel;
    
    count = 0;
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        count++;
    }
    return count;
}

/*
 * irc_server_get_pv_count: return number of pv for server
 */

int
irc_server_get_pv_count (struct t_irc_server *server)
{
    int count;
    struct t_irc_channel *ptr_channel;
    
    count = 0;
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
            count++;
    }
    return count;
}

/*
 * irc_server_remove_away: remove away for all chans/nicks (for all servers)
 */

void
irc_server_remove_away ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
            {
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    irc_channel_remove_away (ptr_channel);
            }
        }
    }
}

/*
 * irc_server_check_away: check for away on all channels (for all servers)
 */

void
irc_server_check_away ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
            {
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    irc_channel_check_away (ptr_server, ptr_channel, 0);
            }
        }
    }
}

/*
 * irc_server_set_away: set/unset away status for a server (all channels)
 */

void
irc_server_set_away (struct t_irc_server *server, const char *nick, int is_away)
{
    struct t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
        if (server->is_connected)
        {
            /* set away flag for nick on channel */
            if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                irc_channel_set_away (ptr_channel, nick, is_away);
            
            /* set/del "away" local variable */
            if (is_away)
            {
                weechat_buffer_set (ptr_channel->buffer,
                                    "localvar_set_away", server->away_message);
            }
            else
            {
                weechat_buffer_set (ptr_channel->buffer,
                                    "localvar_del_away", "");
            }
        }
    }
}

/*
 * irc_server_xfer_send_ready_cb: callback called when user send (file or chat)
 *                                to someone and that xfer plugin successfully
 *                                initialized xfer and is ready for sending
 *                                in that case, irc plugin send message to
 *                                remote nick and wait for "accept" reply
 */

int
irc_server_xfer_send_ready_cb (void *data, const char *signal,
                               const char *type_data, void *signal_data)
{
    struct t_infolist *infolist;
    struct t_irc_server *server, *ptr_server;
    const char *plugin_name, *plugin_id, *type, *filename;
    int spaces_in_name;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    infolist = (struct t_infolist *)signal_data;
    
    if (weechat_infolist_next (infolist))
    {
        plugin_name = weechat_infolist_string (infolist, "plugin_name");
        plugin_id = weechat_infolist_string (infolist, "plugin_id");
        if (plugin_name && (strcmp (plugin_name, IRC_PLUGIN_NAME) == 0) && plugin_id)
        {
            sscanf (plugin_id, "%lx", (long unsigned int *)&server);
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (ptr_server == server)
                    break;
            }
            if (ptr_server)
            {
                type = weechat_infolist_string (infolist, "type");
                if (type)
                {
                    if (strcmp (type, "file_send") == 0)
                    {
                        filename = weechat_infolist_string (infolist, "filename");
                        spaces_in_name = (strchr (filename, ' ') != NULL);
                        irc_server_sendf (server, 
                                          "PRIVMSG %s :\01DCC SEND %s%s%s "
                                          "%s %d %s\01",
                                          weechat_infolist_string (infolist, "remote_nick"),
                                          (spaces_in_name) ? "\"" : "",
                                          filename,
                                          (spaces_in_name) ? "\"" : "",
                                          weechat_infolist_string (infolist, "address"),
                                          weechat_infolist_integer (infolist, "port"),
                                          weechat_infolist_string (infolist, "size"));
                    }
                    else if (strcmp (type, "chat_send") == 0)
                    {
                        irc_server_sendf (server, 
                                          "PRIVMSG %s :\01DCC CHAT chat %s %d\01",
                                          weechat_infolist_string (infolist, "remote_nick"),
                                          weechat_infolist_string (infolist, "address"),
                                          weechat_infolist_integer (infolist, "port"));
                    }
                }
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_xfer_resume_ready_cb: callback called when user receives a file
 *                                  and that resume is possible (file is partially
 *                                  received)
 *                                  in that case, irc plugin send message to
 *                                  remote nick with resume position
 */

int
irc_server_xfer_resume_ready_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    struct t_infolist *infolist;
    struct t_irc_server *server, *ptr_server;
    const char *plugin_name, *plugin_id, *filename;
    int spaces_in_name;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    infolist = (struct t_infolist *)signal_data;
    
    if (weechat_infolist_next (infolist))
    {
        plugin_name = weechat_infolist_string (infolist, "plugin_name");
        plugin_id = weechat_infolist_string (infolist, "plugin_id");
        if (plugin_name && (strcmp (plugin_name, IRC_PLUGIN_NAME) == 0) && plugin_id)
        {
            sscanf (plugin_id, "%lx", (long unsigned int *)&server);
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (ptr_server == server)
                    break;
            }
            if (ptr_server)
            {
                filename = weechat_infolist_string (infolist, "filename");
                spaces_in_name = (strchr (filename, ' ') != NULL);
                irc_server_sendf (server, 
                                  "PRIVMSG %s :\01DCC RESUME %s%s%s %d %s\01",
                                  weechat_infolist_string (infolist, "remote_nick"),
                                  (spaces_in_name) ? "\"" : "",
                                  filename,
                                  (spaces_in_name) ? "\"" : "",
                                  weechat_infolist_integer (infolist, "port"),
                                  weechat_infolist_string (infolist, "start_resume"));
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_xfer_send_accept_resume_cb: callback called when xfer plugin
 *                                        accepted resume request from receiver
 *                                        in that case, irc plugin send accept
 *                                        message to remote nick with resume
 *                                        position
 */

int
irc_server_xfer_send_accept_resume_cb (void *data, const char *signal,
                                       const char *type_data,
                                       void *signal_data)
{
    struct t_infolist *infolist;
    struct t_irc_server *server, *ptr_server;
    const char *plugin_name, *plugin_id, *filename;
    int spaces_in_name;
    
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    infolist = (struct t_infolist *)signal_data;
    
    if (weechat_infolist_next (infolist))
    {
        plugin_name = weechat_infolist_string (infolist, "plugin_name");
        plugin_id = weechat_infolist_string (infolist, "plugin_id");
        if (plugin_name && (strcmp (plugin_name, IRC_PLUGIN_NAME) == 0) && plugin_id)
        {
            sscanf (plugin_id, "%lx", (long unsigned int *)&server);
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (ptr_server == server)
                    break;
            }
            if (ptr_server)
            {
                filename = weechat_infolist_string (infolist, "filename");
                spaces_in_name = (strchr (filename, ' ') != NULL);
                irc_server_sendf (server, 
                                  "PRIVMSG %s :\01DCC ACCEPT %s%s%s %d %s\01",
                                  weechat_infolist_string (infolist, "remote_nick"),
                                  (spaces_in_name) ? "\"" : "",
                                  filename,
                                  (spaces_in_name) ? "\"" : "",
                                  weechat_infolist_integer (infolist, "port"),
                                  weechat_infolist_string (infolist, "start_resume"));
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_server_add_to_infolist: add a server in an infolist
 *                             return 1 if ok, 0 if error
 */

int
irc_server_add_to_infolist (struct t_infolist *infolist,
                            struct t_irc_server *server)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !server)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_string (ptr_item, "name", server->name))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", server->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_name",
                                          (server->buffer) ?
                                          weechat_buffer_get_string (server->buffer, "name") : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_short_name",
                                          (server->buffer) ?
                                          weechat_buffer_get_string (server->buffer, "short_name") : ""))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "selected",
                                           (weechat_config_boolean (irc_config_look_one_server_buffer)
                                            && (irc_current_server != server)) ?
                                           0 : 1))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoconnect", server->autoconnect))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoreconnect", server->autoreconnect))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoreconnect_delay", server->autoreconnect_delay))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "proxy", server->proxy))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "addresses", server->addresses))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv6", server->ipv6))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl", server->ssl))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "password", server->password))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nicks", server->nicks))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "username", server->username))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "realname", server->realname))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_hostname", server->local_hostname))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "command", server->command))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "command_delay", server->command_delay))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "autojoin", server->autojoin))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autorejoin", server->autorejoin))
        return 0;

    if (!weechat_infolist_new_var_integer (ptr_item, "current_address", server->current_address))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", server->sock))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_connected", server->is_connected))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ssl_connected", server->ssl_connected))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "unterminated_message", server->unterminated_message))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nick", server->nick))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nick_modes", server->nick_modes))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefix", server->prefix))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "reconnect_start", server->reconnect_start))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "command_time", server->command_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "reconnect_join", server->reconnect_join))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "disable_autojoin", server->disable_autojoin))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_away", server->is_away))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", server->away_message))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "away_time", server->away_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "lag", server->lag))
        return 0;
    if (!weechat_infolist_new_var_buffer (ptr_item, "lag_check_time", &(server->lag_check_time), sizeof (struct timeval)))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "lag_next_check", server->lag_next_check))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "queue_msg", server->queue_msg))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_user_message", server->last_user_message))
        return 0;
    
    return 1;
}

/*
 * irc_server_print_log: print server infos in log (usually for crash dump)
 */

void
irc_server_print_log ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[server %s (addr:0x%lx)]",      ptr_server->name, ptr_server);
        weechat_log_printf ("  autoconnect . . . . : %d",    ptr_server->autoconnect);
        weechat_log_printf ("  autoreconnect . . . : %d",    ptr_server->autoreconnect);
        weechat_log_printf ("  autoreconnect_delay : %d",    ptr_server->autoreconnect_delay);
        weechat_log_printf ("  proxy . . . . . . . : '%s'",  ptr_server->proxy);
        weechat_log_printf ("  addresses . . . . . : '%s'",  ptr_server->addresses);
        weechat_log_printf ("  ipv6. . . . . . . . : %d",    ptr_server->ipv6);
        weechat_log_printf ("  ssl . . . . . . . . : %d",    ptr_server->ssl);
        weechat_log_printf ("  password. . . . . . : '%s'",
                            (ptr_server->password && ptr_server->password[0]) ?
                            "(hidden)" : ptr_server->password);
        weechat_log_printf ("  nicks . . . . . . . : '%s'",  ptr_server->nicks);
        weechat_log_printf ("  username. . . . . . : '%s'",  ptr_server->username);
        weechat_log_printf ("  realname. . . . . . : '%s'",  ptr_server->realname);
        weechat_log_printf ("  command . . . . . . : '%s'",
                            (ptr_server->command && ptr_server->command[0]) ?
                            "(hidden)" : ptr_server->command);
        weechat_log_printf ("  command_delay . . . : %d",    ptr_server->command_delay);
        weechat_log_printf ("  autojoin. . . . . . : '%s'",  ptr_server->autojoin);
        weechat_log_printf ("  autorejoin. . . . . : %d",    ptr_server->autorejoin);
        weechat_log_printf ("  reloaded_from_config: %d",    ptr_server->reloaded_from_config);
        weechat_log_printf ("  addresses_count . . : %d",    ptr_server->addresses_count);
        weechat_log_printf ("  addresses_array . . : 0x%lx", ptr_server->addresses_array);
        weechat_log_printf ("  ports_array . . . . : 0x%lx", ptr_server->ports_array);
        weechat_log_printf ("  sock. . . . . . . . : %d",    ptr_server->sock);
        weechat_log_printf ("  hook_connect. . . . : 0x%lx", ptr_server->hook_connect);
        weechat_log_printf ("  hook_fd . . . . . . : 0x%lx", ptr_server->hook_fd);
        weechat_log_printf ("  is_connected. . . . : %d",    ptr_server->is_connected);
        weechat_log_printf ("  ssl_connected . . . : %d",    ptr_server->ssl_connected);
        weechat_log_printf ("  unterminated_message: '%s'",  ptr_server->unterminated_message);
        weechat_log_printf ("  nicks_count . . . . : %d",    ptr_server->nicks_count);
        weechat_log_printf ("  nicks_array . . . . : 0x%lx", ptr_server->nicks_array);
        weechat_log_printf ("  nick. . . . . . . . : '%s'",  ptr_server->nick);
        weechat_log_printf ("  nick_modes. . . . . : '%s'",  ptr_server->nick_modes);
        weechat_log_printf ("  prefix. . . . . . . : '%s'",  ptr_server->prefix);
        weechat_log_printf ("  reconnect_start . . : %ld",   ptr_server->reconnect_start);
        weechat_log_printf ("  command_time. . . . : %ld",   ptr_server->command_time);
        weechat_log_printf ("  reconnect_join. . . : %d",    ptr_server->reconnect_join);
        weechat_log_printf ("  disable_autojoin. . : %d",    ptr_server->disable_autojoin);
        weechat_log_printf ("  is_away . . . . . . : %d",    ptr_server->is_away);
        weechat_log_printf ("  away_message. . . . : '%s'",  ptr_server->away_message);
        weechat_log_printf ("  away_time . . . . . : %ld",   ptr_server->away_time);
        weechat_log_printf ("  lag . . . . . . . . : %d",    ptr_server->lag);
        weechat_log_printf ("  lag_check_time. . . : tv_sec:%d, tv_usec:%d",
                            ptr_server->lag_check_time.tv_sec,
                            ptr_server->lag_check_time.tv_usec);
        weechat_log_printf ("  lag_next_check. . . : %ld",   ptr_server->lag_next_check);
        weechat_log_printf ("  last_user_message . : %ld",   ptr_server->last_user_message);
        weechat_log_printf ("  outqueue. . . . . . : 0x%lx", ptr_server->outqueue);
        weechat_log_printf ("  last_outqueue . . . : 0x%lx", ptr_server->last_outqueue);
        weechat_log_printf ("  buffer. . . . . . . : 0x%lx", ptr_server->buffer);
        weechat_log_printf ("  channels. . . . . . : 0x%lx", ptr_server->channels);
        weechat_log_printf ("  last_channel. . . . : 0x%lx", ptr_server->last_channel);
        weechat_log_printf ("  prev_server . . . . : 0x%lx", ptr_server->prev_server);
        weechat_log_printf ("  next_server . . . . : 0x%lx", ptr_server->next_server);

        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_channel_print_log (ptr_channel);
        }
    }
}
