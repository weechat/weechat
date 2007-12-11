/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "irc.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-config.h"
#include "irc-nick.h"
#include "irc-protocol.h"


struct t_irc_server *irc_servers = NULL;
struct t_irc_server *last_irc_server = NULL;

struct t_irc_message *irc_recv_msgq = NULL;
struct t_irc_message *irc_msgq_last_msg = NULL;

#ifdef HAVE_GNUTLS
const int gnutls_cert_type_prio[] = { GNUTLS_CRT_X509, GNUTLS_CRT_OPENPGP, 0 };
#if LIBGNUTLS_VERSION_NUMBER >= 0x010700
    const int gnutls_prot_prio[] = { GNUTLS_TLS1_2, GNUTLS_TLS1_1,
                                     GNUTLS_TLS1_0, GNUTLS_SSL3, 0 };
#else
    const int gnutls_prot_prio[] = { GNUTLS_TLS1_1, GNUTLS_TLS1_0,
                                     GNUTLS_SSL3, 0 };
#endif
#endif


/*
 * irc_server_init: init server struct with default values
 */

void
irc_server_init (struct t_irc_server *server)
{
    /* user choices */
    server->name = NULL;
    server->autoconnect = 0;
    server->autoreconnect = 1;
    server->autoreconnect_delay = 30;
    server->temp_server = 0;
    server->address = NULL;
    server->port = -1;
    server->ipv6 = 0;
    server->ssl = 0;
    server->password = NULL;
    server->nick1 = NULL;
    server->nick2 = NULL;
    server->nick3 = NULL;
    server->username = NULL;
    server->realname = NULL;
    server->hostname = NULL;
    server->command = NULL;
    server->command_delay = 1;
    server->autojoin = NULL;
    server->autorejoin = 0;
    server->notify_levels = NULL;
    
    /* internal vars */
    server->reloaded_from_config = 0;
    server->child_pid = 0;
    server->child_read = -1;
    server->child_write = -1;
    server->sock = -1;
    server->hook_fd = NULL;
    server->is_connected = 0;
    server->ssl_connected = 0;
    server->unterminated_message = NULL;
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
        weechat_config_integer (irc_config_irc_lag_check);
    server->cmd_list_regexp = NULL;
    server->queue_msg = 0;
    server->last_user_message = 0;
    server->outqueue = NULL;
    server->last_outqueue = NULL;
    server->buffer = NULL;
    server->channels = NULL;
    server->last_channel = NULL;
}

/*
 * irc_server_init_with_url: init a server with url of this form:
 *                           irc://nick:pass@irc.toto.org:6667
 *                           returns:  0 = ok
 *                                    -1 = invalid syntax
 */

int
irc_server_init_with_url (struct t_irc_server *server, char *irc_url)
{
    char *url, *pos_server, *pos_channel, *pos, *pos2;
    int ipv6, ssl;
    struct passwd *my_passwd;
    
    irc_server_init (server);
    ipv6 = 0;
    ssl = 0;
    if (strncasecmp (irc_url, "irc6://", 7) == 0)
    {
        pos = irc_url + 7;
        ipv6 = 1;
    }
    else if (strncasecmp (irc_url, "ircs://", 7) == 0)
    {
        pos = irc_url + 7;
        ssl = 1;
    }
    else if ((strncasecmp (irc_url, "irc6s://", 8) == 0)
        || (strncasecmp (irc_url, "ircs6://", 8) == 0))
    {
        pos = irc_url + 8;
        ipv6 = 1;
        ssl = 1;
    }
    else if (strncasecmp (irc_url, "irc://", 6) == 0)
    {
        pos = irc_url + 6;
    }
    else
        return -1;
    
    url = strdup (pos);
    pos_server = strchr (url, '@');
    if (pos_server)
    {
        pos_server[0] = '\0';
        pos_server++;
        if (!pos[0])
        {
            free (url);
            return -1;
        }
        pos2 = strchr (url, ':');
        if (pos2)
        {
            pos2[0] = '\0';
            server->password = strdup (pos2 + 1);
        }
        server->nick1 = strdup (url);
    }
    else
    {
        if ((my_passwd = getpwuid (geteuid ())) != NULL)
            server->nick1 = strdup (my_passwd->pw_name);
        else
        {
            weechat_printf (NULL,
                            _("%sirc: error retrieving user's name: %s"),
                            weechat_prefix ("error"),
                            strerror (errno));
            free (url);
            return -1;
        }
        pos_server = url;
    }
    if (!pos_server[0])
    {
        free (url);
        return -1;
    }
    pos_channel = strchr (pos_server, '/');
    if (pos_channel)
    {
        pos_channel[0] = '\0';
        pos_channel++;
    }
    pos = strchr (pos_server, ':');
    if (pos)
    {
        pos[0] = '\0';
        server->port = atoi (pos + 1);
    }
    server->name = strdup (pos_server);
    server->address = strdup (pos_server);
    if (pos_channel && pos_channel[0])
    {
        if (irc_channel_is_channel (pos_channel))
            server->autojoin = strdup (pos_channel);
        else
        {
            server->autojoin = (char *) malloc (strlen (pos_channel) + 2);
            strcpy (server->autojoin, "#");
            strcat (server->autojoin, pos_channel);
        }
    }
    
    free (url);
    
    server->ipv6 = ipv6;
    server->ssl = ssl;

    /* some default values */
    if (server->port < 0)
        server->port = IRC_SERVER_DEFAULT_PORT;
    server->nick2 = (char *) malloc (strlen (server->nick1) + 2);
    strcpy (server->nick2, server->nick1);
    server->nick2 = strcat (server->nick2, "1");
    server->nick3 = (char *) malloc (strlen (server->nick1) + 2);
    strcpy (server->nick3, server->nick1);
    server->nick3 = strcat (server->nick3, "2");
    
    return 0;
}

/*
 * irc_server_init_with_config_options: init a server with config options
 *                                      (called when reading config file)
 */

void
irc_server_init_with_config_options (struct t_irc_server *server,
                                     void *section,
                                     int config_reload)
{
    struct t_config_option *ptr_option;
    struct t_irc_server *ptr_server;
    
    ptr_option = weechat_config_search_option (NULL, section, "server_name");
    if (!ptr_option)
    {
        irc_server_free (server);
        return;
    }
    
    if (config_reload)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if ((ptr_server != server)
                && (strcmp (ptr_server->name,
                            weechat_config_string (ptr_option)) == 0))
                break;
        }
        if (ptr_server)
            irc_server_free (server);
        else
            ptr_server = server;
    }
    else
        ptr_server = server;
    
    if (ptr_server->name)
        free (ptr_server->name);
    ptr_server->name = strdup (weechat_config_string (ptr_option));
    
    ptr_option = weechat_config_search_option (NULL, section, "server_autoconnect");
    if (ptr_option)
        ptr_server->autoconnect = weechat_config_integer (ptr_option);

    ptr_option = weechat_config_search_option (NULL, section, "server_autoreconnect");
    if (ptr_option)
        ptr_server->autoreconnect = weechat_config_integer (ptr_option);

    ptr_option = weechat_config_search_option (NULL, section, "server_autoreconnect_delay");
    if (ptr_option)
        ptr_server->autoreconnect_delay = weechat_config_integer (ptr_option);

    if (ptr_server->address)
        free (ptr_server->address);
    ptr_server->address = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_address");
    if (ptr_option)
        ptr_server->address = strdup (weechat_config_string (ptr_option));

    ptr_option = weechat_config_search_option (NULL, section, "server_port");
    if (ptr_option)
        ptr_server->port = weechat_config_integer (ptr_option);

    ptr_option = weechat_config_search_option (NULL, section, "server_ipv6");
    if (ptr_option)
        ptr_server->ipv6 = weechat_config_integer (ptr_option);

    ptr_option = weechat_config_search_option (NULL, section, "server_ssl");
    if (ptr_option)
        ptr_server->ssl = weechat_config_integer (ptr_option);

    if (ptr_server->password)
        free (ptr_server->password);
    ptr_server->password = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_password");
    if (ptr_option)
        ptr_server->password = strdup (weechat_config_string (ptr_option));
    
    if (ptr_server->nick1)
        free (ptr_server->nick1);
    ptr_server->nick1 = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_nick1");
    if (ptr_option)
        ptr_server->nick1 = strdup (weechat_config_string (ptr_option));

    if (ptr_server->nick2)
        free (ptr_server->nick2);
    ptr_server->nick2 = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_nick2");
    if (ptr_option)
        ptr_server->nick2 = strdup (weechat_config_string (ptr_option));
    
    if (ptr_server->nick3)
        free (ptr_server->nick3);
    ptr_server->nick3 = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_nick3");
    if (ptr_option)
        ptr_server->nick3 = strdup (weechat_config_string (ptr_option));

    if (ptr_server->username)
        free (ptr_server->username);
    ptr_server->username = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_username");
    if (ptr_option)
        ptr_server->username = strdup (weechat_config_string (ptr_option));
    
    if (ptr_server->realname)
        free (ptr_server->realname);
    ptr_server->realname = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_realname");
    if (ptr_option)
        ptr_server->realname = strdup (weechat_config_string (ptr_option));
    
    if (ptr_server->hostname)
        free (ptr_server->hostname);
    ptr_server->hostname = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_hostname");
    if (ptr_option)
        ptr_server->hostname = strdup (weechat_config_string (ptr_option));
    
    if (ptr_server->command)
        free (ptr_server->command);
    ptr_server->command = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_command");
    if (ptr_option)
        ptr_server->command = strdup (weechat_config_string (ptr_option));

    ptr_option = weechat_config_search_option (NULL, section, "server_command_delay");
    if (ptr_option)
        ptr_server->command_delay = weechat_config_integer (ptr_option);

    if (ptr_server->autojoin)
        free (ptr_server->autojoin);
    ptr_server->autojoin = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_autojoin");
    if (ptr_option)
        ptr_server->autojoin = strdup (weechat_config_string (ptr_option));

    ptr_option = weechat_config_search_option (NULL, section, "server_autorejoin");
    if (ptr_option)
        ptr_server->autorejoin = weechat_config_integer (ptr_option);

    if (ptr_server->notify_levels)
        free (ptr_server->notify_levels);
    ptr_server->notify_levels = NULL;
    ptr_option = weechat_config_search_option (NULL, section, "server_notify_levels");
    if (ptr_option)
        ptr_server->notify_levels = strdup (weechat_config_string (ptr_option));
    
    ptr_server->reloaded_from_config = 1;
}

/*
 * irc_server_alloc: allocate a new server and add it to the servers queue
 */

struct t_irc_server *
irc_server_alloc ()
{
    struct t_irc_server *new_server;
    
    /* alloc memory for new server */
    if ((new_server = (struct t_irc_server *) malloc (sizeof (struct t_irc_server))) == NULL)
    {
        weechat_printf (NULL,
                        _("%sirc: error when allocating new server"),
                        weechat_prefix ("error"));
        return NULL;
    }
    
    /* initialize new server */
    irc_server_init (new_server);
    
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
 * irc_server_outqueue_add: add a message in out queue
 */

void
irc_server_outqueue_add (struct t_irc_server *server, char *msg1, char *msg2,
                         int modified)
{
    struct t_irc_outqueue *new_outqueue;

    new_outqueue = (struct t_irc_outqueue *)malloc (sizeof (struct t_irc_outqueue));
    if (new_outqueue)
    {
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
        irc_server_outqueue_free (server, server->outqueue);
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
    if (server->address)
        free (server->address);
    if (server->password)
        free (server->password);
    if (server->nick1)
        free (server->nick1);
    if (server->nick2)
        free (server->nick2);
    if (server->nick3)
        free (server->nick3);
    if (server->username)
        free (server->username);
    if (server->realname)
        free (server->realname);
    if (server->hostname)
        free (server->hostname);
    if (server->command)
        free (server->command);
    if (server->autojoin)
        free (server->autojoin);
    if (server->notify_levels)
        free (server->notify_levels);
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
        irc_channel_free (server, server->channels);
    
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
irc_server_new (char *name, int autoconnect, int autoreconnect,
                int autoreconnect_delay, int temp_server, char *address,
                int port, int ipv6, int ssl, char *password,
                char *nick1, char *nick2, char *nick3, char *username,
                char *realname, char *hostname, char *command, int command_delay,
                char *autojoin, int autorejoin, char *notify_levels)
{
    struct t_irc_server *new_server;
    
    if (!name || !address || (port < 0))
        return NULL;
    
#ifdef DEBUG
    weechat_log_printf ("Creating new server (name:%s, address:%s, port:%d, pwd:%s, "
                        "nick1:%s, nick2:%s, nick3:%s, username:%s, realname:%s, "
                        "hostname: %s, command:%s, autojoin:%s, autorejoin:%s, "
                        "notify_levels:%s)",
                        name, address, port, (password) ? password : "",
                        (nick1) ? nick1 : "", (nick2) ? nick2 : "", (nick3) ? nick3 : "",
                        (username) ? username : "", (realname) ? realname : "",
                        (hostname) ? hostname : "", (command) ? command : "",
                        (autojoin) ? autojoin : "", (autorejoin) ? "on" : "off",
                        (notify_levels) ? notify_levels : "");
#endif
    
    if ((new_server = irc_server_alloc ()))
    {
        new_server->name = strdup (name);
        new_server->autoconnect = autoconnect;
        new_server->autoreconnect = autoreconnect;
        new_server->autoreconnect_delay = autoreconnect_delay;
        new_server->temp_server = temp_server;
        new_server->address = strdup (address);
        new_server->port = port;
        new_server->ipv6 = ipv6;
        new_server->ssl = ssl;
        new_server->password = (password) ? strdup (password) : strdup ("");
        new_server->nick1 = (nick1) ? strdup (nick1) : strdup ("weechat_user");
        new_server->nick2 = (nick2) ? strdup (nick2) : strdup ("weechat2");
        new_server->nick3 = (nick3) ? strdup (nick3) : strdup ("weechat3");
        new_server->username =
            (username) ? strdup (username) : strdup ("weechat");
        new_server->realname =
            (realname) ? strdup (realname) : strdup ("realname");
        new_server->hostname =
            (hostname) ? strdup (hostname) : NULL;
        new_server->command =
            (command) ? strdup (command) : NULL;
        new_server->command_delay = command_delay;
        new_server->autojoin =
            (autojoin) ? strdup (autojoin) : NULL;
        new_server->autorejoin = autorejoin;
        new_server->notify_levels =
            (notify_levels) ? strdup (notify_levels) : NULL;
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
irc_server_duplicate (struct t_irc_server *server, char *new_name)
{
    struct t_irc_server *new_server;
    
    /* check if another server exists with this name */
    if (irc_server_search (new_name))
        return 0;
    
    /* duplicate server */
    new_server = irc_server_new (new_name,
                                 server->autoconnect,
                                 server->autoreconnect,
                                 server->autoreconnect_delay,
                                 server->temp_server,
                                 server->address,
                                 server->port,
                                 server->ipv6,
                                 server->ssl,
                                 server->password,
                                 server->nick1,
                                 server->nick2,
                                 server->nick3,
                                 server->username,
                                 server->realname,
                                 server->hostname,
                                 server->command,
                                 server->command_delay,
                                 server->autojoin,
                                 server->autorejoin,
                                 server->notify_levels);
    
    return new_server;
}

/*
 * irc_server_rename: rename server (internal name)
 *                    return: 1 if ok, 0 if error
 */

int
irc_server_rename (struct t_irc_server *server, char *new_name)
{
    char *str;
    
    /* check if another server exists with this name */
    if (irc_server_search (new_name))
        return 0;
    
    /* rename server */
    str = strdup (new_name);
    if (str)
    {
        if (server->name)
            free (server->name);
        server->name = str;
        return 1;
    }
    return 0;
}

/*
 * irc_server_send: send data to IRC server
 *                  return number of bytes sent, -1 if error
 */

int
irc_server_send (struct t_irc_server *server, char *buffer, int size_buf)
{
    int rc;
    
    if (!server)
    {
        weechat_printf (NULL,
                        _("%sirc: error sending data to IRC server: null "
                          "pointer (please report problem to developers)"),
                        weechat_prefix ("error"));
        return 0;
    }

    if (size_buf <= 0)
    {
        weechat_printf (server->buffer,
                        _("%sirc: error sending data to IRC server: empty "
                          "buffer (please report problem to "
                          "developers)"),
                        weechat_prefix ("error"));
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
                        _("%sirc: error sending data to IRC server (%s)"),
                        weechat_prefix ("error"),
                        strerror (errno));
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
        if (time_now >= server->last_user_message +
            weechat_config_integer (irc_config_irc_anti_flood))
        {
            if (server->outqueue->message_before_mod)
            {
                pos = strchr (server->outqueue->message_before_mod, '\r');
                if (pos)
                    pos[0] = '\0';
                //gui_chat_printf_raw_data (server, 1, 0,
                //                          server->outqueue->message_before_mod);
                if (pos)
                    pos[0] = '\r';
            }
            if (server->outqueue->message_after_mod)
            {
                pos = strchr (server->outqueue->message_after_mod, '\r');
                if (pos)
                    pos[0] = '\0';
                //gui_chat_printf_raw_data (server, 1, server->outqueue->modified,
                //                          server->outqueue->message_after_mod);
                if (pos)
                    pos[0] = '\r';
            }
            if (irc_server_send (server, server->outqueue->message_after_mod,
                                 strlen (server->outqueue->message_after_mod)) < 0)
            {
                weechat_printf (server->buffer,
                                _("%sirc: error sending data to IRC "
                                  "server"),
                                weechat_prefix ("error"));
            }
            server->last_user_message = time_now;
            irc_server_outqueue_free (server, server->outqueue);
        }
    }
}

/*
 * irc_server_send_one_msg: send one message to IRC server
 */

int
irc_server_send_one_msg (struct t_irc_server *server, char *message)
{
    static char buffer[4096];
    char *new_msg, *ptr_msg, *pos;
    int rc, queue, first_message;
    time_t time_now;
    
    rc = 1;
    
#ifdef DEBUG
    weechat_printf (server->buffer,
                    "[DEBUG] Sending to server >>> %s",
                    message);
#endif
    /*new_msg = plugin_modifier_exec (PLUGIN_MODIFIER_IRC_OUT,
                                    server->name,
                                    message)
    */
    new_msg = NULL;
    
    /* no changes in new message */
    if (new_msg && (strcmp (buffer, new_msg) == 0))
    {
        free (new_msg);
        new_msg = NULL;
    }
    
    /* message not dropped? */
    if (!new_msg || new_msg[0])
    {
        first_message = 1;
        ptr_msg = (new_msg) ? new_msg : message;
        
        while (rc && ptr_msg && ptr_msg[0])
        {
            pos = strchr (ptr_msg, '\n');
            if (pos)
                pos[0] = '\0';
            
            snprintf (buffer, sizeof (buffer) - 1, "%s\r\n", ptr_msg);

            /* anti-flood: look whether we should queue outgoing message or not */
            time_now = time (NULL);
            queue = 0;
            if ((server->queue_msg)
                && ((server->outqueue)
                    || ((weechat_config_integer (irc_config_irc_anti_flood) > 0)
                        && (time_now - server->last_user_message <
                            weechat_config_integer (irc_config_irc_anti_flood)))))
                queue = 1;
            
            /* if queue, then only queue message and send nothing now */
            if (queue)
            {
                irc_server_outqueue_add (server,
                                         (new_msg && first_message) ? message : NULL,
                                         buffer,
                                         (new_msg) ? 1 : 0);
            }
            else
            {
                //if (first_message)
                //    gui_chat_printf_raw_data (server, 1, 0, message);
                //if (new_msg)
                //    gui_chat_printf_raw_data (server, 1, 1, ptr_msg);
                if (irc_server_send (server, buffer, strlen (buffer)) <= 0)
                {
                    weechat_printf (server->buffer,
                                    _("%sirc: error sending data to IRC "
                                      "server"),
                                    weechat_prefix ("error"));
                    rc = 0;
                }
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
    }
    //else
    //    gui_chat_printf_raw_data (server, 1, 1, _("(message dropped)"));
    if (new_msg)
        free (new_msg);
    
    return rc;
}

/*
 * irc_server_sendf: send formatted data to IRC server
 *                   many messages may be sent, separated by '\n'
 */

void
irc_server_sendf (struct t_irc_server *server, char *fmt, ...)
{
    va_list args;
    static char buffer[4096];
    char *ptr_buf, *pos;
    int rc;
    
    if (!server)
        return;
    
    va_start (args, fmt);
    vsnprintf (buffer, sizeof (buffer) - 1, fmt, args);
    va_end (args);    
    
    ptr_buf = buffer;
    while (ptr_buf && ptr_buf[0])
    {
        pos = strchr (ptr_buf, '\n');
        if (pos)
            pos[0] = '\0';
        
        rc = irc_server_send_one_msg (server, ptr_buf);
        
        if (pos)
        {
            pos[0] = '\n';
            ptr_buf = pos + 1;
        }
        else
            ptr_buf = NULL;
        
        if (!rc)
            ptr_buf = NULL;
    }
}

/*
 * irc_server_parse_message: parse IRC message and return pointer to
 *                           host, command and arguments (if any)
 */

void
irc_server_parse_message (char *message, char **host, char **command, char **args)
{
    char *pos, *pos2;
    
    *host = NULL;
    *command = NULL;
    *args = NULL;
    
    if (message[0] == ':')
    {
        pos = strchr (message, ' ');
        if (pos)
        {
            *host = strndup (message + 1, pos - (message + 1));
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
            pos++;
        pos2 = strchr (pos, ' ');
        if (pos2)
        {
            *command = strndup (pos, pos2 - pos);
            pos2++;
            while (pos2[0] == ' ')
                pos2++;
            *args = strdup (pos2);
        }
    }
}

/*
 * irc_server_msgq_add_msg: add a message to received messages queue (at the end)
 */

void
irc_server_msgq_add_msg (struct t_irc_server *server, char *msg)
{
    struct t_irc_message *message;
    
    if (!server->unterminated_message && !msg[0])
        return;
    
    message = (struct t_irc_message *) malloc (sizeof (struct t_irc_message));
    if (!message)
    {
        weechat_printf (server->buffer,
                        _("%sirc: not enough memory for received IRC "
                          "message"),
                        weechat_prefix ("error"));
        return;
    }
    message->server = server;
    if (server->unterminated_message)
    {
        message->data = (char *) malloc (strlen (server->unterminated_message) +
                                         strlen (msg) + 1);
        if (!message->data)
        {
            weechat_printf (server->buffer,
                            _("%sirc: not enough memory for received IRC "
                              "message"),
                            weechat_prefix ("error"));
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
irc_server_msgq_add_unterminated (struct t_irc_server *server, char *string)
{
    if (!string[0])
        return;
    
    if (server->unterminated_message)
    {
        server->unterminated_message =
            (char *) realloc (server->unterminated_message,
                              strlen (server->unterminated_message) +
                              strlen (string) + 1);
        if (!server->unterminated_message)
        {
            weechat_printf (server->buffer,
                            _("%sirc: not enough memory for received IRC "
                              "message"),
                            weechat_prefix ("error"));
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
                            _("%sirc: not enough memory for received IRC "
                              "message"),
                            weechat_prefix ("error"));
        }
    }
}

/*
 * irc_server_msgq_add_buffer: explode received buffer, creating queued messages
 */

void
irc_server_msgq_add_buffer (struct t_irc_server *server, char *buffer)
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
    char *host, *command, *args;
    
    while (irc_recv_msgq)
    {
        if (irc_recv_msgq->data)
        {
#ifdef DEBUG
            weechat_printf (irc_recv_msgq->server->buffer,
                            "[DEBUG] %s",
                            irc_recv_msgq->data);
#endif
            ptr_data = irc_recv_msgq->data;
            while (ptr_data[0] == ' ')
                ptr_data++;
            
            if (ptr_data[0])
            {
                //gui_chat_printf_raw_data (irc_recv_msgq->server, 0, 0,
                //                          ptr_data);
#ifdef DEBUG
                weechat_printf (irc_recv_msgq->server->buffer,
                                "[DEBUG] data received from server: %s",
                                ptr_data);
#endif
                /*new_msg = plugin_modifier_exec (PLUGIN_MODIFIER_IRC_IN,
                                                irc_recv_msgq->server->name,
                                                ptr_data);*/
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

                        //if (new_msg)
                        //    gui_chat_printf_raw_data (irc_recv_msgq->server,
                        //                              0, 1, ptr_msg);
                        
                        irc_server_parse_message (ptr_msg, &host,
                                                  &command, &args);
                        
                        switch (irc_protocol_recv_command (irc_recv_msgq->server,
                                                           ptr_msg,
                                                           host, command, args))
                        {
                            case -1:
                                weechat_printf (irc_recv_msgq->server->buffer,
                                                _("%sirc: command \"%s\" "
                                                  "failed!"),
                                                weechat_prefix ("error"),
                                                command);
                                break;
                            case -2:
                                weechat_printf (irc_recv_msgq->server->buffer,
                                                _("%sirc: no command to "
                                                  "execute!"),
                                                weechat_prefix ("error"));
                                break;
                            case -3:
                                weechat_printf (irc_recv_msgq->server->buffer,
                                                _("%sirc: unknown command: "
                                                  "cmd=\"%s\", "
                                                  "host=\"%s\", "
                                                  "args=\"%s\""),
                                                weechat_prefix ("error"),
                                                command, host, args);
                                break;
                        }
                        
                        if (host)
                            free (host);
                        if (command)
                            free (command);
                        if (args)
                            free (args);
                        
                        if (pos)
                        {
                            pos[0] = '\n';
                            ptr_msg = pos + 1;
                        }
                        else
                            ptr_msg = NULL;
                    }
                }
                //else
                //    gui_chat_printf_raw_data (irc_recv_msgq->server, 0, 1,
                //                              _("(message dropped)"));
                if (new_msg)
                    free (new_msg);
            }
            free (irc_recv_msgq->data);
        }
        
        next = irc_recv_msgq->next_message;
        free (irc_recv_msgq);
        irc_recv_msgq = next;
        if (irc_recv_msgq == NULL)
            irc_msgq_last_msg = NULL;
    }
}

/*
 * irc_server_recv: receive data from an irc server
 */

int
irc_server_recv (void *arg_server)
{
    struct t_irc_server *server;

    server = (struct t_irc_server *)arg_server;
    
    static char buffer[4096 + 2];
    int num_read;

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
                        _("%sirc: cannot read data from socket, "
                          "disconnecting from server..."),
                        weechat_prefix ("error"));
        irc_server_disconnect (server, 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * irc_server_timer: timer called each second to perform some operations
 *                   on servers
 */

void
irc_server_timer (void *empty)
{
    struct t_irc_server *ptr_server;
    time_t new_time;
    static struct timeval tv;
    int diff;
    
    (void) empty;
    
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
                    irc_server_sendf (ptr_server, "PING %s", ptr_server->address);
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
                    && (weechat_config_integer (irc_config_irc_lag_disconnect) > 0))
                {
                    gettimeofday (&tv, NULL);
                    diff = (int) weechat_timeval_diff (&(ptr_server->lag_check_time),
                                                       &tv);
                    if (diff / 1000 > weechat_config_integer (irc_config_irc_lag_disconnect) * 60)
                    {
                        weechat_printf (ptr_server->buffer,
                                        _("%sirc: lag is high, "
                                          "disconnecting from "
                                          "server..."),
                                        weechat_prefix ("info"));
                        irc_server_disconnect (ptr_server, 1);
                    }
                }
            }
        }
    }
}

/*
 * irc_server_timer_check_away: timer called to check away on servers
 *                              (according to option "irc_check_away")
 */

void
irc_server_timer_check_away (void *empty)
{
    (void) empty;
    
    if (weechat_config_integer (irc_config_irc_away_check) > 0)
        irc_server_check_away ();
}

/*
 * irc_server_child_kill: kill child process and close pipe
 */

void
irc_server_child_kill (struct t_irc_server *server)
{
    /* kill process */
    if (server->child_pid > 0)
    {
        kill (server->child_pid, SIGKILL);
        waitpid (server->child_pid, NULL, 0);
        server->child_pid = 0;
    }
    
    /* close pipe used with child */
    if (server->child_read != -1)
    {
        close (server->child_read);
        server->child_read = -1;
    }
    if (server->child_write != -1)
    {
        close (server->child_write);
        server->child_write = -1;
    }
}

/*
 * irc_server_close_connection: close server connection
 *                              (kill child, close socket/pipes)
 */

void
irc_server_close_connection (struct t_irc_server *server)
{
    irc_server_child_kill (server);
    
    /* close network socket */
    if (server->sock != -1)
    {
#ifdef HAVE_GNUTLS
        if (server->ssl_connected)
            gnutls_bye (server->gnutls_sess, GNUTLS_SHUT_WR);
#endif
        close (server->sock);
        server->sock = -1;
#ifdef HAVE_GNUTLS
        if (server->ssl_connected)
            gnutls_deinit (server->gnutls_sess);
#endif
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
    if (server->autoreconnect)
    {
        server->reconnect_start = time (NULL);
        weechat_printf (server->buffer,
                        _("%sirc: reconnecting to server in %d seconds"),
                        weechat_prefix ("info"),
                        server->autoreconnect_delay);
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
        server->nick = strdup (server->nick1);
    irc_server_sendf (server,
                      "NICK %s\n"
                      "USER %s %s %s :%s",
                      server->nick, server->username, server->username,
                      server->address, server->realname);
    //gui_input_draw (weechat_current_buffer, 1);
}

/*
 * irc_server_child_read: read connection progress from child process
 */

int
irc_server_child_read (void *arg_server)
{
    struct t_irc_server *server;
    char buffer[1];
    int num_read;
    int config_proxy_use;
    
    server = (struct t_irc_server *)arg_server;
    
    num_read = read (server->child_read, buffer, sizeof (buffer));
    if (num_read > 0)
    {
        config_proxy_use = weechat_config_boolean (weechat_config_get ("proxy_use"));
        switch (buffer[0])
        {
            /* connection OK */
            case '0':
                /* enable SSL if asked */
#ifdef HAVE_GNUTLS
                if (server->ssl_connected)
                {
                    gnutls_transport_set_ptr (server->gnutls_sess,
                                              (gnutls_transport_ptr) ((unsigned long) server->sock));
                    if (gnutls_handshake (server->gnutls_sess) < 0)
                    {
                        weechat_printf (server->buffer,
                                        _("%sirc: GnuTLS handshake failed"),
                                        weechat_prefix ("error"));
                        irc_server_close_connection (server);
                        irc_server_reconnect_schedule (server);
                        return WEECHAT_RC_OK;
                    }
                }
#endif
                /* kill child and login to server */
                irc_server_child_kill (server);
                irc_server_login (server);
                weechat_unhook (server->hook_fd);
                server->hook_fd = weechat_hook_fd (server->sock,
                                                   1, 0, 0,
                                                   irc_server_recv,
                                                   server);
                break;
            /* adress not found */
            case '1':
                weechat_printf (server->buffer,
                                (config_proxy_use) ?
                                _("%sirc: proxy address \"%s\" not found") :
                                _("%sirc: address \"%s\" not found"),
                                weechat_prefix ("error"),
                                server->address);
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* IP address not found */
            case '2':
                weechat_printf (server->buffer,
                                (config_proxy_use) ?
                                _("%sirc: proxy IP address not found") :
                                _("%sirc: IP address not found"),
                                weechat_prefix ("error"));
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* connection refused */
            case '3':
                weechat_printf (server->buffer,
                                (config_proxy_use) ?
                                _("%sirc: proxy connection refused") :
                                _("%sirc: connection refused"),
                                weechat_prefix ("error"));
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* proxy fails to connect to server */
            case '4':
                weechat_printf (server->buffer,
                                _("%sirc: proxy fails to establish "
                                  "connection to server "
                                  "(check username/password if used)"),
                                weechat_prefix ("error"));
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* fails to set local hostname/IP */
            case '5':
                weechat_printf (server->buffer,
                                _("%sirc: unable to set local hostname/IP"),
                                weechat_prefix ("error"));
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
        }
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_server_convbase64_8x3_to_6x4 : convert 3 bytes of 8 bits in 4 bytes of 6 bits
 */

void
irc_server_convbase64_8x3_to_6x4 (char *from, char* to)
{
    unsigned char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    to[0] = base64_table [ (from[0] & 0xfc) >> 2 ];
    to[1] = base64_table [ ((from[0] & 0x03) << 4) + ((from[1] & 0xf0) >> 4) ];
    to[2] = base64_table [ ((from[1] & 0x0f) << 2) + ((from[2] & 0xc0) >> 6) ];
    to[3] = base64_table [ from[2] & 0x3f ];
}

/*
 * irc_server_base64encode: encode a string in base64
 */

void
irc_server_base64encode (char *from, char *to)
{
    char *f, *t;
    int from_len;
    
    from_len = strlen(from);
    
    f = from;
    t = to;
    
    while (from_len >= 3)
    {
        irc_server_convbase64_8x3_to_6x4 (f, t);
        f += 3 * sizeof (*f);
        t += 4 * sizeof (*t);
        from_len -= 3;
    }
    
    if (from_len > 0)
    {
        char rest[3] = { 0, 0, 0 };
        switch (from_len)
        {
            case 1 :
                rest[0] = f[0];
                irc_server_convbase64_8x3_to_6x4 (rest, t);
                t[2] = t[3] = '=';
                break;
            case 2 :
                rest[0] = f[0];
                rest[1] = f[1];
                irc_server_convbase64_8x3_to_6x4 (rest, t);
                t[3] = '=';
                break;
        }
        t[4] = 0;
    }
}

/*
 * irc_server_pass_httpproxy: establish connection/authentification to an
 *                            http proxy
 *                            return : 
 *                             - 0 if connexion throw proxy was successful
 *                             - 1 if connexion fails
 */

int
irc_server_pass_httpproxy (int sock, char *address, int port)
{

    char buffer[256], authbuf[128], authbuf_base64[196];
    char *config_proxy_username, *config_proxy_password;
    int n, m;

    config_proxy_username = weechat_config_string (weechat_config_get ("proxy_username"));
    config_proxy_username = weechat_config_string (weechat_config_get ("proxy_password"));
    
    if (config_proxy_username && config_proxy_username[0])
    {
        /* authentification */
        snprintf (authbuf, sizeof (authbuf), "%s:%s",
                  config_proxy_username,
                  (config_proxy_password) ? config_proxy_password : "");
        irc_server_base64encode (authbuf, authbuf_base64);
        n = snprintf (buffer, sizeof (buffer),
                      "CONNECT %s:%d HTTP/1.0\r\nProxy-Authorization: Basic %s\r\n\r\n",
                      address, port, authbuf_base64);
    }
    else 
    {
        /* no authentification */
        n = snprintf (buffer, sizeof (buffer),
                      "CONNECT %s:%d HTTP/1.0\r\n\r\n", address, port);
    }
    
    m = send (sock, buffer, n, 0);
    if (n != m)
        return 1;
    
    n = recv (sock, buffer, sizeof (buffer), 0);
    
    /* success result must be like: "HTTP/1.0 200 OK" */
    if (n < 12)
        return 1;
    
    if (memcmp (buffer, "HTTP/", 5) || memcmp (buffer + 9, "200", 3))
        return 1;
    
    return 0;
}

/*
 * irc_server_resolve: resolve hostname on its IP address
 *                     (works with ipv4 and ipv6)
 *                     return :
 *                      - 0 if resolution was successful
 *                      - 1 if resolution fails
 */

int
irc_server_resolve (char *hostname, char *ip, int *version)
{
    char ipbuffer[NI_MAXHOST];
    struct addrinfo *res;
    
    if (version != NULL)
        *version = 0;
    
    res = NULL;
    
    if (getaddrinfo (hostname, NULL, NULL, &res) != 0)
        return 1;
    
    if (!res)
        return 1;
    
    if (getnameinfo (res->ai_addr, res->ai_addrlen, ipbuffer, sizeof(ipbuffer), NULL, 0, NI_NUMERICHOST) != 0)
    {
        freeaddrinfo (res);
        return 1;
    }
    
    if ((res->ai_family == AF_INET) && (version != NULL))
        *version = 4;
    if ((res->ai_family == AF_INET6) && (version != NULL))
        *version = 6;
    
    strcpy (ip, ipbuffer);
    
    freeaddrinfo (res);
    
    return 0;
}

/*
 * irc_server_pass_socks4proxy: establish connection/authentification thru a
 *                              socks4 proxy
 *                              return : 
 *                               - 0 if connexion thru proxy was successful
 *                               - 1 if connexion fails
 */

int
irc_server_pass_socks4proxy (int sock, char *address, int port, char *username)
{
    /* 
     * socks4 protocol is explained here: 
     *  http://archive.socks.permeo.com/protocol/socks4.protocol
     *
     */
    
    struct s_socks4
    {
        char version;           /* 1 byte  */ /* socks version : 4 or 5 */
        char method;            /* 1 byte  */ /* socks method : connect (1) or bind (2) */
        unsigned short port;    /* 2 bytes */ /* destination port */
        unsigned long address;  /* 4 bytes */ /* destination address */
        char user[64];          /* username (64 characters seems to be enought) */
    } socks4;
    unsigned char buffer[24];
    char ip_addr[NI_MAXHOST];
    
    socks4.version = 4;
    socks4.method = 1;
    socks4.port = htons (port);
    irc_server_resolve (address, ip_addr, NULL);
    socks4.address = inet_addr (ip_addr);
    strncpy (socks4.user, username, sizeof (socks4.user) - 1);
    
    send (sock, (char *) &socks4, 8 + strlen (socks4.user) + 1, 0);
    recv (sock, buffer, sizeof (buffer), 0);
    
    if (buffer[0] == 0 && buffer[1] == 90)
        return 0;
    
    return 1;
}

/*
 * irc_server_pass_socks5proxy: establish connection/authentification thru a
 *                              socks5 proxy
 *                              return : 
 *                               - 0 if connexion thru proxy was successful
 *                               - 1 if connexion fails
 */

int
irc_server_pass_socks5proxy (int sock, char *address, int port)
{
    /* 
     * socks5 protocol is explained in RFC 1928
     * socks5 authentication with username/pass is explained in RFC 1929
     */
    
    struct s_sock5
    {
        char version;   /* 1 byte      */ /* socks version : 4 or 5 */
        char nmethods;  /* 1 byte      */ /* size in byte(s) of field 'method', here 1 byte */
        char method;    /* 1-255 bytes */ /* socks method : noauth (0), auth(user/pass) (2), ... */
    } socks5;
    unsigned char buffer[288];
    int username_len, password_len, addr_len, addr_buffer_len;
    unsigned char *addr_buffer;
    char *config_proxy_username, *config_proxy_password;
    
    socks5.version = 5;
    socks5.nmethods = 1;
    
    config_proxy_username = weechat_config_string (weechat_config_get ("proxy_username"));
    config_proxy_username = weechat_config_string (weechat_config_get ("proxy_password"));
    
    if (config_proxy_username && config_proxy_username[0])
        socks5.method = 2; /* with authentication */
    else
        socks5.method = 0; /* without authentication */
    
    send (sock, (char *) &socks5, sizeof(socks5), 0);
    /* server socks5 must respond with 2 bytes */
    if (recv (sock, buffer, 2, 0) != 2)
        return 1;
    
    if (config_proxy_username && config_proxy_username[0])
    {
        /* with authentication */
        /*   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 2 => authentication
         */
        
        if (buffer[0] != 5 || buffer[1] != 2)
            return 1;
        
        /* authentication as in RFC 1929 */
        username_len = strlen (config_proxy_username);
        password_len = strlen (config_proxy_password);
        
        /* make username/password buffer */
        buffer[0] = 1;
        buffer[1] = (unsigned char) username_len;
        memcpy(buffer + 2, config_proxy_username, username_len);
        buffer[2 + username_len] = (unsigned char) password_len;
        memcpy (buffer + 3 + username_len, config_proxy_password, password_len);
        
        send (sock, buffer, 3 + username_len + password_len, 0);
        
        /* server socks5 must respond with 2 bytes */
        if (recv (sock, buffer, 2, 0) != 2)
            return 1;
        
        /* buffer[1] = auth state, must be 0 for success */
        if (buffer[1] != 0)
            return 1;
    }
    else
    {
        /* without authentication */
        /*   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 0 => no authentication
         */
        if (!(buffer[0] == 5 && buffer[1] == 0))
            return 1;
    }
    
    /* authentication successful then giving address/port to connect */
    addr_len = strlen(address);
    addr_buffer_len = 4 + 1 + addr_len + 2;
    addr_buffer = (unsigned char *) malloc (addr_buffer_len * sizeof(*addr_buffer));
    if (!addr_buffer)
        return 1;
    addr_buffer[0] = 5;   /* version 5 */
    addr_buffer[1] = 1;   /* command: 1 for connect */
    addr_buffer[2] = 0;   /* reserved */
    addr_buffer[3] = 3;   /* address type : ipv4 (1), domainname (3), ipv6 (4) */
    addr_buffer[4] = (unsigned char) addr_len;
    memcpy (addr_buffer + 5, address, addr_len); /* server address */
    *((unsigned short *) (addr_buffer + 5 + addr_len)) = htons (port); /* server port */
    
    send (sock, addr_buffer, addr_buffer_len, 0);
    free (addr_buffer);
    
    /* dialog with proxy server */
    if (recv (sock, buffer, 4, 0) != 4)
        return 1;
    
    if (!(buffer[0] == 5 && buffer[1] == 0))
        return 1;
    
    /* buffer[3] = address type */
    switch (buffer[3])
    {
        case 1 :
            /* ipv4 
             * server socks return server bound address and port
             * address of 4 bytes and port of 2 bytes (= 6 bytes)
             */
            if (recv (sock, buffer, 6, 0) != 6)
                return 1;
            break;
        case 3:
            /* domainname
             * server socks return server bound address and port
             */
            /* reading address length */
            if (recv (sock, buffer, 1, 0) != 1)
                return 1;    
            addr_len = buffer[0];
            /* reading address + port = addr_len + 2 */
            if (recv (sock, buffer, addr_len + 2, 0) != (addr_len + 2))
                return 1;
            break;
        case 4 :
            /* ipv6
             * server socks return server bound address and port
             * address of 16 bytes and port of 2 bytes (= 18 bytes)
             */
            if (recv (sock, buffer, 18, 0) != 18)
                return 1;
            break;
        default:
            return 1;
    }
    
    return 0;
}

/*
 * irc_server_pass_proxy: establish connection/authentification to a proxy
 *                        return : 
 *                         - 0 if connexion throw proxy was successful
 *                         - 1 if connexion fails
 */

int
irc_server_pass_proxy (int sock, char *address, int port, char *username)
{
    int rc;
    char *config_proxy_type;
    
    config_proxy_type = weechat_config_string (weechat_config_get ("proxy_type"));
    
    rc = 1;
    if (config_proxy_type)
    {
        if (weechat_strcasecmp (config_proxy_type, "http") == 0)
            rc = irc_server_pass_httpproxy (sock, address, port);
        if (weechat_strcasecmp (config_proxy_type,  "socks4") == 0)
            rc = irc_server_pass_socks4proxy (sock, address, port, username);
        if (weechat_strcasecmp (config_proxy_type, "socks5") == 0)
            rc = irc_server_pass_socks5proxy (sock, address, port);
    }
    return rc;
}

/*
 * irc_server_child: child process trying to connect to server
 */

int
irc_server_child (struct t_irc_server *server)
{
    struct addrinfo hints, *res, *res_local;
    int rc;
    int config_proxy_use, config_proxy_ipv6, config_proxy_port;
    char *config_proxy_address;
    
    res = NULL;
    res_local = NULL;

    config_proxy_use = weechat_config_boolean (weechat_config_get ("proxy_use"));
    config_proxy_ipv6 = weechat_config_integer (weechat_config_get ("proxy_ipv6"));
    config_proxy_port = weechat_config_integer (weechat_config_get ("proxy_port"));
    config_proxy_address = weechat_config_string (weechat_config_get ("proxy_address"));
    
    if (config_proxy_use)
    {
        /* get info about server */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = (config_proxy_ipv6) ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo (config_proxy_address, NULL, &hints, &res) !=0)
        {
            write (server->child_write, "1", 1);
            return 0;
        }
        if (!res)
        {
            write (server->child_write, "1", 1);
            return 0;
        }
        if ((config_proxy_ipv6 && (res->ai_family != AF_INET6))
            || ((!config_proxy_ipv6 && (res->ai_family != AF_INET))))
        {
            write (server->child_write, "2", 1);
            freeaddrinfo (res);
            return 0;
        }
        
        if (config_proxy_ipv6)
            ((struct sockaddr_in6 *)(res->ai_addr))->sin6_port = htons (config_proxy_port);
        else
            ((struct sockaddr_in *)(res->ai_addr))->sin_port = htons (config_proxy_port);

        /* connect to server */
        if (connect (server->sock, res->ai_addr, res->ai_addrlen) != 0)
        {
            write (server->child_write, "3", 1);
            freeaddrinfo (res);
            return 0;
        }
        
        if (irc_server_pass_proxy (server->sock, server->address, server->port, server->username))
        {
            write (server->child_write, "4", 1);
            freeaddrinfo (res);
            return 0;
        }
    }
    else
    {
        /* set local hostname/IP if asked by user */
        if (server->hostname && server->hostname[0])
        {
            memset (&hints, 0, sizeof(hints));
            hints.ai_family = (server->ipv6) ? AF_INET6 : AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            rc = getaddrinfo (server->hostname, NULL, &hints, &res_local);
            if ((rc != 0) || !res_local
                || (server->ipv6 && (res_local->ai_family != AF_INET6))
                || ((!server->ipv6 && (res_local->ai_family != AF_INET))))
            {
                write (server->child_write, "5", 1);
                if (res_local)
                    freeaddrinfo (res_local);
                return 0;
            }
            if (bind (server->sock, res_local->ai_addr, res_local->ai_addrlen) < 0)
            {
                write (server->child_write, "5", 1);
                if (res_local)
                    freeaddrinfo (res_local);
                return 0;
            }
        }
        
        /* get info about server */
        memset (&hints, 0, sizeof(hints));
        hints.ai_family = (server->ipv6) ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        rc = getaddrinfo (server->address, NULL, &hints, &res);
        if ((rc != 0) || !res)
        {
            write (server->child_write, "1", 1);
            if (res)
                freeaddrinfo (res);
            return 0;
        }
        if ((server->ipv6 && (res->ai_family != AF_INET6))
            || ((!server->ipv6 && (res->ai_family != AF_INET))))
        {
            write (server->child_write, "2", 1);
            if (res)
                freeaddrinfo (res);
            if (res_local)
                freeaddrinfo (res_local);
            return 0;
        }
        
        /* connect to server */
        if (server->ipv6)
            ((struct sockaddr_in6 *)(res->ai_addr))->sin6_port = htons (server->port);
        else
            ((struct sockaddr_in *)(res->ai_addr))->sin_port = htons (server->port);
        
        if (connect (server->sock, res->ai_addr, res->ai_addrlen) != 0)
        {
            write (server->child_write, "3", 1);
            if (res)
                freeaddrinfo (res);
            if (res_local)
                freeaddrinfo (res_local);
            return 0;
        }
    }
    
    write (server->child_write, "0", 1);
    if (res)
        freeaddrinfo (res);
    if (res_local)
        freeaddrinfo (res_local);
    return 0;
}

/*
 * irc_server_connect: connect to an IRC server
 *                     Return: 1 if ok
 *                             0 if error
 */

int
irc_server_connect (struct t_irc_server *server, int disable_autojoin)
{
    int child_pipe[2], set;
#ifndef __CYGWIN__
    pid_t pid;
#endif
    char *config_proxy_type, *config_proxy_address;
    int config_proxy_use, config_proxy_ipv6, config_proxy_port;

    config_proxy_use = weechat_config_boolean (weechat_config_get ("proxy_use"));
    config_proxy_ipv6 = weechat_config_boolean (weechat_config_get ("proxy_ipv6"));
    config_proxy_type = weechat_config_string (weechat_config_get ("proxy_type"));
    config_proxy_address = weechat_config_string (weechat_config_get ("proxy_address"));
    config_proxy_port = weechat_config_integer (weechat_config_get ("proxy_port"));
    
    if (!server->buffer)
    {
        server->buffer = weechat_buffer_new (server->name, server->name, NULL);
        if (!server->buffer)
            return 0;
        weechat_buffer_set (server->buffer, "display", "1");
    }
    
#ifndef HAVE_GNUTLS
    if (server->ssl)
    {
        weechat_printf (server->buffer,
                        _("%sirc: cannot connect with SSL since WeeChat "
                          "was not built with GnuTLS support"),
                        weechat_prefix ("error"));
        return 0;
    }
#endif
    if (config_proxy_use)
    {
        weechat_printf (server->buffer,
                        _("%sirc: connecting to server %s:%d%s%s via %s "
                          "proxy %s:%d%s..."),
                        weechat_prefix ("info"),
                        server->address, server->port,
                        (server->ipv6) ? " (IPv6)" : "",
                        (server->ssl) ? " (SSL)" : "",
                        config_proxy_type,
                        config_proxy_address, config_proxy_port,
                        (config_proxy_ipv6) ? " (IPv6)" : "");
        weechat_log_printf (_("Connecting to server %s:%d%s%s via %s proxy "
                              "%s:%d%s..."),
                            server->address, server->port,
                            (server->ipv6) ? " (IPv6)" : "",
                            (server->ssl) ? " (SSL)" : "",
                            config_proxy_type,
                            config_proxy_address, config_proxy_port,
                            (config_proxy_ipv6) ? " (IPv6)" : "");
    }
    else
    {
        weechat_printf (server->buffer,
                        _("%sirc: connecting to server %s:%d%s%s..."),
                        weechat_prefix ("info"),
                        server->address, server->port,
                        (server->ipv6) ? " (IPv6)" : "",
                        (server->ssl) ? " (SSL)" : "");
        weechat_log_printf (_("irc: connecting to server %s:%d%s%s..."),
                            server->address, server->port,
                            (server->ipv6) ? " (IPv6)" : "",
                            (server->ssl) ? " (SSL)" : "");
    }
    
    /* close any opened connection and kill child process if running */
    irc_server_close_connection (server);
    
    /* init SSL if asked */
    server->ssl_connected = 0;
#ifdef HAVE_GNUTLS
    if (server->ssl)
    {
        if (gnutls_init (&server->gnutls_sess, GNUTLS_CLIENT) != 0)
        {
            weechat_printf (server->buffer,
                            _("%sirc: GnuTLS init error"),
                            weechat_prefix ("error"));
            return 0;
        }
        gnutls_set_default_priority (server->gnutls_sess);
        gnutls_certificate_type_set_priority (server->gnutls_sess,
                                              gnutls_cert_type_prio);
        gnutls_protocol_set_priority (server->gnutls_sess, gnutls_prot_prio);
        gnutls_credentials_set (server->gnutls_sess, GNUTLS_CRD_CERTIFICATE,
                                gnutls_xcred);
        server->ssl_connected = 1;
    }
#endif
    
    /* create pipe for child process */
    if (pipe (child_pipe) < 0)
    {
        weechat_printf (server->buffer,
                        _("%sirc: cannot create pipe"),
                        weechat_prefix ("error"));
        return 0;
    }
    server->child_read = child_pipe[0];
    server->child_write = child_pipe[1];
    
    /* create socket and set options */
    if (config_proxy_use)
      server->sock = socket ((config_proxy_ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    else
      server->sock = socket ((server->ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (server->sock == -1)
    {
        weechat_printf (server->buffer,
                        _("%sirc: cannot create socket"),
                        weechat_prefix ("error"));
        return 0;
    }
    
    /* set SO_REUSEADDR option for socket */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_REUSEADDR,
        (void *) &set, sizeof (set)) == -1)
    {
        weechat_printf (server->buffer,
                        _("%sirc: cannot set socket option "
                          "\"SO_REUSEADDR\""),
                        weechat_prefix ("error"));
    }
    
    /* set SO_KEEPALIVE option for socket */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_KEEPALIVE,
        (void *) &set, sizeof (set)) == -1)
    {
        weechat_printf (server->buffer,
                        _("%sirc: cannot set socket option "
                          "\"SO_KEEPALIVE\""),
                        weechat_prefix ("error"));
    }

#ifdef __CYGWIN__
    /* connection may block under Cygwin, there's no other known way
       to do better today, since connect() in child process seems not to work
       any suggestion is welcome to improve that!
    */
    irc_server_child (server);
    server->child_pid = 0;
    irc_server_child_read (server);
#else
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            irc_server_close_connection (server);
            return 0;
        /* child process */
        case 0:
            setuid (getuid ());
            irc_server_child (server);
            _exit (EXIT_SUCCESS);
    }
    /* parent process */
    server->child_pid = pid;
    server->hook_fd = weechat_hook_fd (server->child_read,
                                       1, 0, 0,
                                       irc_server_child_read,
                                       server);
#endif
    
    server->disable_autojoin = disable_autojoin;
    
    return 1;
}

/*
 * irc_server_reconnect: reconnect to a server (after disconnection)
 */

void
irc_server_reconnect (struct t_irc_server *server)
{
    weechat_printf (server->buffer,
                    _("%sirc: reconnecting to server..."),
                    weechat_prefix ("info"));
    server->reconnect_start = 0;
    
    if (irc_server_connect (server, 0))
        server->reconnect_join = 1;
    else
        irc_server_reconnect_schedule (server);
}

/*
 * irc_server_auto_connect: auto-connect to servers (called at startup)
 */

void
irc_server_auto_connect (int auto_connect, int temp_server)
{
    struct t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if ( ((temp_server) && (ptr_server->temp_server))
            || ((!temp_server) && (auto_connect) && (ptr_server->autoconnect)) )
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
            //irc_nick_free_all (ptr_channel);
            weechat_printf (ptr_channel->buffer,
                            _("%sirc: disconnected from server!"),
                            weechat_prefix ("info"));
            //gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            //gui_status_draw (ptr_channel->buffer, 1);
        }
    }
    
    irc_server_close_connection (server);
    
    if (server->buffer)
        weechat_printf (server->buffer,
                        _("%sirc: disconnected from server!"),
                        weechat_prefix ("info"));
    
    if (server->nick_modes)
    {
        free (server->nick_modes);
        server->nick_modes = NULL;
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
        weechat_config_integer (irc_config_irc_lag_check);
    
    if ((reconnect) && (server->autoreconnect))
        irc_server_reconnect_schedule (server);
    else
        server->reconnect_start = 0;
    
    /* discard current nick if no reconnection asked */
    if (!reconnect && server->nick)
    {
        free (server->nick);
        server->nick = NULL;
    }
    
    //gui_window_redraw_buffer (weechat_current_buffer);
}

/*
 * irc_server_disconnect_all: disconnect from all irc servers
 */

void
irc_server_disconnect_all ()
{
    struct t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        irc_server_disconnect (ptr_server, 0);
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
        //if (!server->disable_autojoin && server->autojoin && server->autojoin[0])
        //    irc_cmd_join_server (server, server->autojoin);
    }

    server->disable_autojoin = 0;
}

/*
 * irc_server_search: return pointer on a server with a name
 */

struct t_irc_server *
irc_server_search (char *servername)
{
    struct t_irc_server *ptr_server;
    
    if (!servername)
        return NULL;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (strcmp (ptr_server->name, servername) == 0)
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
 * irc_server_name_already_exists: return 1 if server name already exists
 *                                 otherwise return 0
 */

int
irc_server_name_already_exists (char *name)
{
    struct t_irc_server *ptr_server;
    
    if (!name)
        return 0;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (strcmp (ptr_server->name, name) == 0)
            return 1;
    }
    return 0;
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
irc_server_set_away (struct t_irc_server *server, char *nick, int is_away)
{
    struct t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
        if (server->is_connected)
        {
            if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                irc_channel_set_away (ptr_channel, nick, is_away);
        }
    }
}

/*
 * irc_server_get_default_notify_level: get default notify level for server
 */

int
irc_server_get_default_notify_level (struct t_irc_server *server)
{
    (void) server;
    
    /*int notify, value;
    char *pos;

    notify = GUI_NOTIFY_LEVEL_DEFAULT;
    
    if (!server || !server->notify_levels)
        return notify;
    
    pos = strstr (server->notify_levels, "*:");
    if (pos)
    {
        pos += 2;
        if (pos[0])
        {
            value = (int)(pos[0] - '0');
            if ((value >= GUI_NOTIFY_LEVEL_MIN)
                && (value <= GUI_NOTIFY_LEVEL_MAX))
                notify = value;
        }
    }

    return notify;*/
    return 0;
}

/*
 * irc_server_set_default_notify_level: set default notify level for server
 */

void
irc_server_set_default_notify_level (struct t_irc_server *server, int notify)
{
    (void) server;
    (void) notify;
    
    /*char level_string[2];

    if (server)
    {
        level_string[0] = notify + '0';
        level_string[1] = '\0';
        config_option_list_set (&(server->notify_levels), "*", level_string);
    }
    */
}

/*
 * irc_server_print_log: print server infos in log (usually for crash dump)
 */

void
irc_server_print_log ()
{
    struct t_irc_server *ptr_server;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[server %s (addr:0x%X)]",      ptr_server->name, ptr_server);
        weechat_log_printf ("  autoconnect . . . . : %d",   ptr_server->autoconnect);
        weechat_log_printf ("  autoreconnect . . . : %d",   ptr_server->autoreconnect);
        weechat_log_printf ("  autoreconnect_delay : %d",   ptr_server->autoreconnect_delay);
        weechat_log_printf ("  temp_server . . . . : %d",   ptr_server->temp_server);
        weechat_log_printf ("  address . . . . . . : '%s'", ptr_server->address);
        weechat_log_printf ("  port. . . . . . . . : %d",   ptr_server->port);
        weechat_log_printf ("  ipv6. . . . . . . . : %d",   ptr_server->ipv6);
        weechat_log_printf ("  ssl . . . . . . . . : %d",   ptr_server->ssl);
        weechat_log_printf ("  password. . . . . . : '%s'",
                            (ptr_server->password && ptr_server->password[0]) ?
                            "(hidden)" : ptr_server->password);
        weechat_log_printf ("  nick1 . . . . . . . : '%s'", ptr_server->nick1);
        weechat_log_printf ("  nick2 . . . . . . . : '%s'", ptr_server->nick2);
        weechat_log_printf ("  nick3 . . . . . . . : '%s'", ptr_server->nick3);
        weechat_log_printf ("  username. . . . . . : '%s'", ptr_server->username);
        weechat_log_printf ("  realname. . . . . . : '%s'", ptr_server->realname);
        weechat_log_printf ("  command . . . . . . : '%s'",
                            (ptr_server->command && ptr_server->command[0]) ?
                            "(hidden)" : ptr_server->command);
        weechat_log_printf ("  command_delay . . . : %d",   ptr_server->command_delay);
        weechat_log_printf ("  autojoin. . . . . . : '%s'", ptr_server->autojoin);
        weechat_log_printf ("  autorejoin. . . . . : %d",   ptr_server->autorejoin);
        weechat_log_printf ("  notify_levels . . . : %s",   ptr_server->notify_levels);
        weechat_log_printf ("  reloaded_from_config: %d",   ptr_server->reloaded_from_config);
        weechat_log_printf ("  child_pid . . . . . : %d",   ptr_server->child_pid);
        weechat_log_printf ("  child_read  . . . . : %d",   ptr_server->child_read);
        weechat_log_printf ("  child_write . . . . : %d",   ptr_server->child_write);
        weechat_log_printf ("  sock. . . . . . . . : %d",   ptr_server->sock);
        weechat_log_printf ("  hook_fd . . . . . . : 0x%X", ptr_server->hook_fd);
        weechat_log_printf ("  is_connected. . . . : %d",   ptr_server->is_connected);
        weechat_log_printf ("  ssl_connected . . . : %d",   ptr_server->ssl_connected);
        weechat_log_printf ("  unterminated_message: '%s'", ptr_server->unterminated_message);
        weechat_log_printf ("  nick. . . . . . . . : '%s'", ptr_server->nick);
        weechat_log_printf ("  nick_modes. . . . . : '%s'", ptr_server->nick_modes);
        weechat_log_printf ("  prefix. . . . . . . : '%s'", ptr_server->prefix);
        weechat_log_printf ("  reconnect_start . . : %ld",  ptr_server->reconnect_start);
        weechat_log_printf ("  command_time. . . . : %ld",  ptr_server->command_time);
        weechat_log_printf ("  reconnect_join. . . : %d",   ptr_server->reconnect_join);
        weechat_log_printf ("  disable_autojoin. . : %d",   ptr_server->disable_autojoin);
        weechat_log_printf ("  is_away . . . . . . : %d",   ptr_server->is_away);
        weechat_log_printf ("  away_message. . . . : '%s'", ptr_server->away_message);
        weechat_log_printf ("  away_time . . . . . : %ld",  ptr_server->away_time);
        weechat_log_printf ("  lag . . . . . . . . : %d",   ptr_server->lag);
        weechat_log_printf ("  lag_check_time. . . : tv_sec:%d, tv_usec:%d",
                            ptr_server->lag_check_time.tv_sec,
                            ptr_server->lag_check_time.tv_usec);
        weechat_log_printf ("  lag_next_check. . . : %ld",  ptr_server->lag_next_check);
        weechat_log_printf ("  last_user_message . : %ld",  ptr_server->last_user_message);
        weechat_log_printf ("  outqueue. . . . . . : 0x%X", ptr_server->outqueue);
        weechat_log_printf ("  last_outqueue . . . : 0x%X", ptr_server->last_outqueue);
        weechat_log_printf ("  buffer. . . . . . . : 0x%X", ptr_server->buffer);
        weechat_log_printf ("  channels. . . . . . : 0x%X", ptr_server->channels);
        weechat_log_printf ("  last_channel. . . . : 0x%X", ptr_server->last_channel);
        weechat_log_printf ("  prev_server . . . . : 0x%X", ptr_server->prev_server);
        weechat_log_printf ("  next_server . . . . : 0x%X", ptr_server->next_server);
    }
}
