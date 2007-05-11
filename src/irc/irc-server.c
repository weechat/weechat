/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "../common/weechat.h"
#include "irc.h"
#include "../common/log.h"
#include "../common/util.h"
#include "../common/weeconfig.h"
#include "../gui/gui.h"

#ifdef PLUGINS
#include "../plugins/plugins.h"
#endif


t_irc_server *irc_servers = NULL;
t_irc_server *last_irc_server = NULL;

t_irc_message *recv_msgq, *msgq_last_msg;

int check_away = 0;

char *nick_modes = "aiwroOs";

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
irc_server_init (t_irc_server *server)
{
    /* user choices */
    server->name = NULL;
    server->autoconnect = 0;
    server->autoreconnect = 1;
    server->autoreconnect_delay = 30;
    server->command_line = 0;
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
    server->child_pid = 0;
    server->child_read = -1;
    server->child_write = -1;
    server->sock = -1;
    server->is_connected = 0;
    server->ssl_connected = 0;
    server->unterminated_message = NULL;
    server->nick = NULL;
    server->nick_modes = NULL;
    server->prefix = NULL;
    server->reconnect_start = 0;
    server->reconnect_join = 0;
    server->is_away = 0;
    server->away_message = NULL;
    server->away_time = 0;
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) + cfg_irc_lag_check;
    server->cmd_list_regexp = NULL;
    server->queue_msg = 0;
    server->last_user_message = 0;
    server->outqueue = NULL;
    server->last_outqueue = NULL;
    server->buffer = NULL;
    server->saved_buffer = NULL;
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
irc_server_init_with_url (char *irc_url, t_irc_server *server)
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
            weechat_iconv_fprintf (stderr, "%s: %s (%s).",
                                   WEECHAT_WARNING,
                                   _("Unable to get user's name"),
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
        server->port = DEFAULT_IRC_PORT;
    server->nick2 = (char *) malloc (strlen (server->nick1) + 2);
    strcpy (server->nick2, server->nick1);
    server->nick2 = strcat (server->nick2, "1");
    server->nick3 = (char *) malloc (strlen (server->nick1) + 2);
    strcpy (server->nick3, server->nick1);
    server->nick3 = strcat (server->nick3, "2");
    
    return 0;
}

/*
 * irc_server_alloc: allocate a new server and add it to the servers queue
 */

t_irc_server *
irc_server_alloc ()
{
    t_irc_server *new_server;
    
    /* alloc memory for new server */
    if ((new_server = (t_irc_server *) malloc (sizeof (t_irc_server))) == NULL)
    {
        weechat_iconv_fprintf (stderr,
                               _("%s cannot allocate new server\n"),
                               WEECHAT_ERROR);
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
irc_server_outqueue_add (t_irc_server *server, char *msg1, char *msg2,
                         int modified)
{
    t_irc_outqueue *new_outqueue;

    new_outqueue = (t_irc_outqueue *)malloc (sizeof (t_irc_outqueue));
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
irc_server_outqueue_free (t_irc_server *server, t_irc_outqueue *outqueue)
{
    t_irc_outqueue *new_outqueue;
    
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
irc_server_outqueue_free_all (t_irc_server *server)
{
    while (server->outqueue)
        irc_server_outqueue_free (server, server->outqueue);
}

/*
 * irc_server_destroy: free server data (not struct himself)
 */

void
irc_server_destroy (t_irc_server *server)
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
irc_server_free (t_irc_server *server)
{
    t_irc_server *new_irc_servers;

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
    
    irc_server_destroy (server);
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

t_irc_server *
irc_server_new (char *name, int autoconnect, int autoreconnect,
                int autoreconnect_delay, int command_line, char *address,
                int port, int ipv6, int ssl, char *password,
                char *nick1, char *nick2, char *nick3, char *username,
                char *realname, char *hostname, char *command, int command_delay,
                char *autojoin, int autorejoin, char *notify_levels)
{
    t_irc_server *new_server;
    
    if (!name || !address || (port < 0))
        return NULL;
    
#ifdef DEBUG
    weechat_log_printf ("Creating new server (name:%s, address:%s, port:%d, pwd:%s, "
                        "nick1:%s, nick2:%s, nick3:%s, username:%s, realname:%s, "
                        "hostname: %s, command:%s, autojoin:%s, autorejoin:%s, "
                        "notify_levels:%s)\n",
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
        new_server->command_line = command_line;
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
 * irc_server_send: send data to IRC server
 */

int
irc_server_send (t_irc_server *server, char *buffer, int size_buf)
{
    if (!server)
        return -1;
    
#ifdef HAVE_GNUTLS
    if (server->ssl_connected)
        return gnutls_record_send (server->gnutls_sess, buffer, size_buf);
    else
#endif
        return send (server->sock, buffer, size_buf, 0);
}

/*
 * irc_server_outqueue_send: send a message from outqueue
 */

void
irc_server_outqueue_send (t_irc_server *server)
{
    time_t time_now;
    char *pos;
    
    if (server->outqueue)
    {
        time_now = time (NULL);
        if (time_now >= server->last_user_message + cfg_irc_anti_flood)
        {
            if (server->outqueue->message_before_mod)
            {
                pos = strchr (server->outqueue->message_before_mod, '\r');
                if (pos)
                    pos[0] = '\0';
                gui_printf_raw_data (server, 1, 0,
                                     server->outqueue->message_before_mod);
                if (pos)
                    pos[0] = '\r';
            }
            if (server->outqueue->message_after_mod)
            {
                pos = strchr (server->outqueue->message_after_mod, '\r');
                if (pos)
                    pos[0] = '\0';
                gui_printf_raw_data (server, 1, server->outqueue->modified,
                                     server->outqueue->message_after_mod);
                if (pos)
                    pos[0] = '\r';
            }
            if (irc_server_send (server, server->outqueue->message_after_mod,
                                 strlen (server->outqueue->message_after_mod)) <= 0)
            {
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf (server->buffer, _("%s error sending data to IRC server\n"),
                            WEECHAT_ERROR);
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
irc_server_send_one_msg (t_irc_server *server, char *message)
{
    static char buffer[4096];
    char *new_msg, *ptr_msg, *pos;
    int rc, queue, first_message;
    time_t time_now;
    
    rc = 1;
    
#ifdef DEBUG
    gui_printf (server->buffer, "[DEBUG] Sending to server >>> %s\n", message);
#endif
#ifdef PLUGINS
    new_msg = plugin_modifier_exec (PLUGIN_MODIFIER_IRC_OUT,
                                    server->name,
                                    message);
#else
    new_msg = NULL;
#endif
    
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
                    || ((cfg_irc_anti_flood > 0)
                        && (time_now - server->last_user_message < cfg_irc_anti_flood))))
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
                if (first_message)
                    gui_printf_raw_data (server, 1, 0, message);
                if (new_msg)
                    gui_printf_raw_data (server, 1, 1, ptr_msg);
                if (irc_server_send (server, buffer, strlen (buffer)) <= 0)
                {
                    irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                    gui_printf (server->buffer, _("%s error sending data to IRC server\n"),
                                WEECHAT_ERROR);
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
    else
        gui_printf_raw_data (server, 1, 1, _("(message dropped)"));
    if (new_msg)
        free (new_msg);
    
    return rc;
}

/*
 * irc_server_sendf: send formatted data to IRC server
 *                   many messages may be sent, separated by '\n'
 */

void
irc_server_sendf (t_irc_server *server, char *fmt, ...)
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
irc_server_msgq_add_msg (t_irc_server *server, char *msg)
{
    t_irc_message *message;
    
    if (!server->unterminated_message && !msg[0])
        return;
    
    message = (t_irc_message *) malloc (sizeof (t_irc_message));
    if (!message)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s not enough memory for received IRC message\n"),
                    WEECHAT_ERROR);
        return;
    }
    message->server = server;
    if (server->unterminated_message)
    {
        message->data = (char *) malloc (strlen (server->unterminated_message) +
                                         strlen (msg) + 1);
        if (!message->data)
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf (server->buffer,
                        _("%s not enough memory for received IRC message\n"),
                        WEECHAT_ERROR);
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

    if (msgq_last_msg)
    {
        msgq_last_msg->next_message = message;
        msgq_last_msg = message;
    }
    else
    {
        recv_msgq = message;
        msgq_last_msg = message;
    }
}

/*
 * irc_server_msgq_add_unterminated: add an unterminated message to queue
 */

void
irc_server_msgq_add_unterminated (t_irc_server *server, char *string)
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
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf (server->buffer,
                        _("%s not enough memory for received IRC message\n"),
                        WEECHAT_ERROR);
        }
        else
            strcat (server->unterminated_message, string);
    }
    else
    {
        server->unterminated_message = strdup (string);
        if (!server->unterminated_message)
        {
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf (server->buffer,
                        _("%s not enough memory for received IRC message\n"),
                        WEECHAT_ERROR);
        }
    }
}

/*
 * irc_server_msgq_add_buffer: explode received buffer, creating queued messages
 */

void
irc_server_msgq_add_buffer (t_irc_server *server, char *buffer)
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
    t_irc_message *next;
    char *ptr_data, *new_msg, *ptr_msg, *pos;
    char *host, *command, *args;
    
    while (recv_msgq)
    {
        if (recv_msgq->data)
        {
#ifdef DEBUG
            gui_printf (gui_current_window->buffer, "[DEBUG] %s\n", recv_msgq->data);
#endif
            ptr_data = recv_msgq->data;
            while (ptr_data[0] == ' ')
                ptr_data++;
            
            if (ptr_data[0])
            {
                gui_printf_raw_data (recv_msgq->server, 0, 0, ptr_data);
#ifdef DEBUG
                gui_printf (NULL, "[DEBUG] data received from server: %s\n", ptr_data);
#endif
#ifdef PLUGINS
                new_msg = plugin_modifier_exec (PLUGIN_MODIFIER_IRC_IN,
                                                recv_msgq->server->name,
                                                ptr_data);
#else
                new_msg = NULL;
#endif
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
                            gui_printf_raw_data (recv_msgq->server, 0, 1, ptr_msg);
                        
                        irc_server_parse_message (ptr_msg, &host, &command, &args);
                        
                        switch (irc_recv_command (recv_msgq->server, ptr_msg, host, command, args))
                        {
                            case -1:
                                irc_display_prefix (recv_msgq->server,
                                                    recv_msgq->server->buffer, PREFIX_ERROR);
                                gui_printf (recv_msgq->server->buffer,
                                            _("%s Command \"%s\" failed!\n"), WEECHAT_ERROR, command);
                                break;
                            case -2:
                                irc_display_prefix (recv_msgq->server,
                                                    recv_msgq->server->buffer, PREFIX_ERROR);
                                gui_printf (recv_msgq->server->buffer,
                                            _("%s No command to execute!\n"), WEECHAT_ERROR);
                                break;
                            case -3:
                                irc_display_prefix (recv_msgq->server,
                                                    recv_msgq->server->buffer, PREFIX_ERROR);
                                gui_printf (recv_msgq->server->buffer,
                                            _("%s Unknown command: cmd=\"%s\", host=\"%s\", args=\"%s\"\n"),
                                            WEECHAT_WARNING, command, host, args);
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
                else
                    gui_printf_raw_data (recv_msgq->server, 0, 1, _("(message dropped)"));
                if (new_msg)
                    free (new_msg);
            }
            free (recv_msgq->data);
        }
        
        next = recv_msgq->next_message;
        free (recv_msgq);
        recv_msgq = next;
        if (recv_msgq == NULL)
            msgq_last_msg = NULL;
    }
}

/*
 * irc_server_recv: receive data from an irc server
 */

void
irc_server_recv (t_irc_server *server)
{
    static char buffer[4096 + 2];
    int num_read;

    if (!server)
        return;
    
#ifdef HAVE_GNUTLS
    if (server->ssl_connected)
        num_read = gnutls_record_recv (server->gnutls_sess, buffer, sizeof (buffer) - 2);
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
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot read data from socket, disconnecting from server...\n"),
                    WEECHAT_ERROR);
        irc_server_disconnect (server, 1);
    }
}

/*
 * irc_server_child_kill: kill child process and close pipe
 */

void
irc_server_child_kill (t_irc_server *server)
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
irc_server_close_connection (t_irc_server *server)
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
irc_server_reconnect_schedule (t_irc_server *server)
{
    if (server->autoreconnect)
    {
        server->reconnect_start = time (NULL);
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("%s: Reconnecting to server in %d seconds\n"),
                    PACKAGE_NAME, server->autoreconnect_delay);
    }
    else
        server->reconnect_start = 0;
}

/*
 * irc_server_child_read: read connection progress from child process
 */

void
irc_server_child_read (t_irc_server *server)
{
    char buffer[1];
    int num_read;
    
    num_read = read (server->child_read, buffer, sizeof (buffer));
    if (num_read > 0)
    {
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
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf (server->buffer,
                                    _("%s gnutls handshake failed\n"),
                                    WEECHAT_ERROR);
                        irc_server_close_connection (server);
                        irc_server_reconnect_schedule (server);
                        return;
                    }
                }
#endif
                /* kill child and login to server */
                irc_server_child_kill (server);
                irc_send_login (server);
                break;
            /* adress not found */
            case '1':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                if (cfg_proxy_use)
                    gui_printf (server->buffer,
                                _("%s proxy address \"%s\" not found\n"),
                                WEECHAT_ERROR, server->address);
                else
                    gui_printf (server->buffer,
                                _("%s address \"%s\" not found\n"),
                                WEECHAT_ERROR, server->address);
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* IP address not found */
            case '2':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                if (cfg_proxy_use)
                    gui_printf (server->buffer,
                                _("%s proxy IP address not found\n"), WEECHAT_ERROR);
                else
                    gui_printf (server->buffer,
                                _("%s IP address not found\n"), WEECHAT_ERROR);
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* connection refused */
            case '3':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                if (cfg_proxy_use)
                    gui_printf (server->buffer,
                                _("%s proxy connection refused\n"), WEECHAT_ERROR);
                else
                    gui_printf (server->buffer,
                                _("%s connection refused\n"), WEECHAT_ERROR);
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* proxy fails to connect to server */
            case '4':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf (server->buffer,
                            _("%s proxy fails to establish connection to "
                              "server (check username/password if used)\n"),
                            WEECHAT_ERROR);
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
            /* fails to set local hostname/IP */
            case '5':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf (server->buffer,
                            _("%s unable to set local hostname/IP\n"),
                            WEECHAT_ERROR);
                irc_server_close_connection (server);
                irc_server_reconnect_schedule (server);
                break;
        }
    }
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

    char buffer[256];
    char authbuf[128];
    char authbuf_base64[196];
    int n, m;
    
    if (cfg_proxy_username && cfg_proxy_username[0])
    {
        /* authentification */
        snprintf (authbuf, sizeof (authbuf), "%s:%s",
                  cfg_proxy_username, cfg_proxy_password);
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
    
    socks5.version = 5;
    socks5.nmethods = 1;
    
    if (cfg_proxy_username && cfg_proxy_username[0])
        socks5.method = 2; /* with authentication */
    else
        socks5.method = 0; /* without authentication */
    
    send (sock, (char *) &socks5, sizeof(socks5), 0);
    /* server socks5 must respond with 2 bytes */
    if (recv (sock, buffer, 2, 0) != 2)
        return 1;
    
    if (cfg_proxy_username && cfg_proxy_username[0])
    {
        /* with authentication */
        /*   -> socks server must respond with :
         *       - socks version (buffer[0]) = 5 => socks5
         *       - socks method  (buffer[1]) = 2 => authentication
         */
        
        if (buffer[0] != 5 || buffer[1] != 2)
            return 1;
        
        /* authentication as in RFC 1929 */
        username_len = strlen(cfg_proxy_username);
        password_len = strlen(cfg_proxy_password);
        
        /* make username/password buffer */
        buffer[0] = 1;
        buffer[1] = (unsigned char) username_len;
        memcpy(buffer + 2, cfg_proxy_username, username_len);
        buffer[2 + username_len] = (unsigned char) password_len;
        memcpy (buffer + 3 + username_len, cfg_proxy_password, password_len);
        
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
    switch(buffer[3])
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
    if (strcmp (cfg_proxy_type_values[cfg_proxy_type], "http") == 0)
        return irc_server_pass_httpproxy (sock, address, port);
    if (strcmp (cfg_proxy_type_values[cfg_proxy_type], "socks4") == 0)
        return irc_server_pass_socks4proxy (sock, address, port, username);
    if (strcmp (cfg_proxy_type_values[cfg_proxy_type], "socks5") == 0)
        return irc_server_pass_socks5proxy (sock, address, port);
    
    return 1;
}

/*
 * irc_server_child: child process trying to connect to server
 */

int
irc_server_child (t_irc_server *server)
{
    struct addrinfo hints, *res, *res_local;
    int rc;
    
    res = NULL;
    res_local = NULL;
    
    if (cfg_proxy_use)
    {
        /* get info about server */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = (cfg_proxy_ipv6) ? AF_INET6 : AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo (cfg_proxy_address, NULL, &hints, &res) !=0)
        {
            write(server->child_write, "1", 1);
            return 0;
        }
        if (!res)
        {
            write(server->child_write, "1", 1);
            return 0;
        }
        if ((cfg_proxy_ipv6 && (res->ai_family != AF_INET6))
            || ((!cfg_proxy_ipv6 && (res->ai_family != AF_INET))))
        {
            write (server->child_write, "2", 1);
            freeaddrinfo (res);
            return 0;
        }
        
        if (cfg_proxy_ipv6)
            ((struct sockaddr_in6 *)(res->ai_addr))->sin6_port = htons (cfg_proxy_port);
        else
            ((struct sockaddr_in *)(res->ai_addr))->sin_port = htons (cfg_proxy_port);

        /* connect to server */
        if (connect (server->sock, res->ai_addr, res->ai_addrlen) != 0)
        {
            write(server->child_write, "3", 1);
            freeaddrinfo (res);
            return 0;
        }
        
        if (irc_server_pass_proxy (server->sock, server->address, server->port, server->username))
        {
            write(server->child_write, "4", 1);
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
 */

int
irc_server_connect (t_irc_server *server)
{
    int child_pipe[2], set;
    pid_t pid;
    
#ifndef HAVE_GNUTLS
    if (server->ssl)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot connect with SSL since WeeChat was not built "
                    "with GNUtls support\n"), WEECHAT_ERROR);
        return 0;
    }
#endif
    irc_display_prefix (server, server->buffer, PREFIX_INFO);
    if (cfg_proxy_use)
      {
          gui_printf (server->buffer,
                      _("%s: connecting to server %s:%d%s%s via %s proxy %s:%d%s...\n"),
                      PACKAGE_NAME, server->address, server->port,
                      (server->ipv6) ? " (IPv6)" : "",
                      (server->ssl) ? " (SSL)" : "",
                      cfg_proxy_type_values[cfg_proxy_type], cfg_proxy_address, cfg_proxy_port,
                      (cfg_proxy_ipv6) ? " (IPv6)" : "");
          weechat_log_printf (_("Connecting to server %s:%d%s%s via %s proxy %s:%d%s...\n"),
                            server->address, server->port,
                            (server->ipv6) ? " (IPv6)" : "",
                            (server->ssl) ? " (SSL)" : "",
                            cfg_proxy_type_values[cfg_proxy_type], cfg_proxy_address, cfg_proxy_port,
                            (cfg_proxy_ipv6) ? " (IPv6)" : "");
      }
    else
      {
          gui_printf (server->buffer,
                      _("%s: connecting to server %s:%d%s%s...\n"),
                      PACKAGE_NAME, server->address, server->port,
                      (server->ipv6) ? " (IPv6)" : "",
                      (server->ssl) ? " (SSL)" : "");
          weechat_log_printf (_("Connecting to server %s:%d%s%s...\n"),
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
            irc_display_prefix (server, server->buffer, PREFIX_ERROR);
            gui_printf (server->buffer,
                        _("%s gnutls init error\n"), WEECHAT_ERROR);
            return 0;
        }
        gnutls_set_default_priority (server->gnutls_sess);
        gnutls_certificate_type_set_priority (server->gnutls_sess, gnutls_cert_type_prio);
        gnutls_protocol_set_priority (server->gnutls_sess, gnutls_prot_prio);
        gnutls_credentials_set (server->gnutls_sess, GNUTLS_CRD_CERTIFICATE, gnutls_xcred);
        server->ssl_connected = 1;
    }
#endif
    
    /* create pipe for child process */
    if (pipe (child_pipe) < 0)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot create pipe\n"), WEECHAT_ERROR);
        return 0;
    }
    server->child_read = child_pipe[0];
    server->child_write = child_pipe[1];
    
    /* create socket and set options */
    if (cfg_proxy_use)
      server->sock = socket ((cfg_proxy_ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    else
      server->sock = socket ((server->ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (server->sock == -1)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot create socket\n"), WEECHAT_ERROR);
        return 0;
    }
    
    /* set SO_REUSEADDR option for socket */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_REUSEADDR,
        (void *) &set, sizeof (set)) == -1)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot set socket option \"SO_REUSEADDR\"\n"),
                    WEECHAT_WARNING);
    }
    
    /* set SO_KEEPALIVE option for socket */
    set = 1;
    if (setsockopt (server->sock, SOL_SOCKET, SO_KEEPALIVE,
        (void *) &set, sizeof (set)) == -1)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot set socket option \"SO_KEEPALIVE\"\n"),
                    WEECHAT_WARNING);
    }
    
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
    
    return 1;
}

/*
 * irc_server_reconnect: reconnect to a server (after disconnection)
 */

void
irc_server_reconnect (t_irc_server *server)
{
    irc_display_prefix (server, server->buffer, PREFIX_INFO);
    gui_printf (server->buffer, _("%s: Reconnecting to server...\n"),
                PACKAGE_NAME);
    server->reconnect_start = 0;
    
    if (irc_server_connect (server))
        server->reconnect_join = 1;
    else
        irc_server_reconnect_schedule (server);
}

/*
 * irc_server_auto_connect: auto-connect to servers (called at startup)
 */

void
irc_server_auto_connect (int auto_connect, int command_line)
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if ( ((command_line) && (ptr_server->command_line))
            || ((!command_line) && (auto_connect) && (ptr_server->autoconnect)) )
        {
            (void) gui_buffer_new (gui_current_window, ptr_server, NULL,
                                   BUFFER_TYPE_STANDARD, 1);
            gui_window_redraw_buffer (gui_current_window->buffer);
            if (!irc_server_connect (ptr_server))
                irc_server_reconnect_schedule (ptr_server);
        }
    }
}

/*
 * irc_server_disconnect: disconnect from an irc server
 */

void
irc_server_disconnect (t_irc_server *server, int reconnect)
{
    t_irc_channel *ptr_channel;
    
    if (server->is_connected)
    {
        /* write disconnection message on each channel/private buffer */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_nick_free_all (ptr_channel);
            irc_display_prefix (NULL, ptr_channel->buffer, PREFIX_INFO);
            gui_printf (ptr_channel->buffer, _("Disconnected from server!\n"));
            gui_nicklist_draw (ptr_channel->buffer, 1, 1);
            gui_status_draw (ptr_channel->buffer, 1);
        }
    }
    
    irc_server_close_connection (server);
    
    if (server->buffer)
    {
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Disconnected from server!\n"));
    }
    
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
    server->lag_next_check = time (NULL) + cfg_irc_lag_check;
    
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
    
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * irc_server_disconnect_all: disconnect from all irc servers
 */

void
irc_server_disconnect_all ()
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        irc_server_disconnect (ptr_server, 0);
}

/*
 * irc_server_search: return pointer on a server with a name
 */

t_irc_server *
irc_server_search (char *servername)
{
    t_irc_server *ptr_server;
    
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
    t_irc_server *ptr_server;
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
irc_server_get_number_buffer (t_irc_server *server,
                              int *server_pos, int *server_total)
{
    t_irc_server *ptr_server;
    
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
    t_irc_server *ptr_server;
    
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
 * irc_server_remove_away: remove away for all chans/nicks (for all servers)
 */

void
irc_server_remove_away ()
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
            {
                if (ptr_channel->type == CHANNEL_TYPE_CHANNEL)
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
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
        {
            for (ptr_channel = ptr_server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
            {
                if (ptr_channel->type == CHANNEL_TYPE_CHANNEL)
                    irc_channel_check_away (ptr_server, ptr_channel, 0);
            }
        }
    }
}

/*
 * irc_server_set_away: set/unset away status for a server (all channels)
 */

void
irc_server_set_away (t_irc_server *server, char *nick, int is_away)
{
    t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
        if (server->is_connected)
        {
            if (ptr_channel->type == CHANNEL_TYPE_CHANNEL)
                irc_channel_set_away (ptr_channel, nick, is_away);
        }
    }
}

/*
 * irc_server_get_default_notify_level: get default notify level for server
 */

int
irc_server_get_default_notify_level (t_irc_server *server)
{
    int notify, value;
    char *pos;

    notify = NOTIFY_LEVEL_DEFAULT;
    
    if (!server || !server->notify_levels)
        return notify;
    
    pos = strstr (server->notify_levels, "*:");
    if (pos)
    {
        pos += 2;
        if (pos[0])
        {
            value = (int)(pos[0] - '0');
            if ((value >= NOTIFY_LEVEL_MIN) && (value <= NOTIFY_LEVEL_MAX))
                notify = value;
        }
    }

    return notify;
}

/*
 * irc_server_set_default_notify_level: set default notify level for server
 */

void
irc_server_set_default_notify_level (t_irc_server *server, int notify)
{
    char level_string[2];

    if (server)
    {
        level_string[0] = notify + '0';
        level_string[1] = '\0';
        config_option_list_set (&(server->notify_levels), "*", level_string);
    }
}

/*
 * irc_server_print_log: print server infos in log (usually for crash dump)
 */

void
irc_server_print_log (t_irc_server *server)
{
    weechat_log_printf ("[server %s (addr:0x%X)]\n",      server->name, server);
    weechat_log_printf ("  autoconnect . . . . : %d\n",   server->autoconnect);
    weechat_log_printf ("  autoreconnect . . . : %d\n",   server->autoreconnect);
    weechat_log_printf ("  autoreconnect_delay : %d\n",   server->autoreconnect_delay);
    weechat_log_printf ("  command_line. . . . : %d\n",   server->command_line);
    weechat_log_printf ("  address . . . . . . : '%s'\n", server->address);
    weechat_log_printf ("  port. . . . . . . . : %d\n",   server->port);
    weechat_log_printf ("  ipv6. . . . . . . . : %d\n",   server->ipv6);
    weechat_log_printf ("  ssl . . . . . . . . : %d\n",   server->ssl);
    weechat_log_printf ("  password. . . . . . : '%s'\n",
                        (server->password && server->password[0]) ?
                        "(hidden)" : server->password);
    weechat_log_printf ("  nick1 . . . . . . . : '%s'\n", server->nick1);
    weechat_log_printf ("  nick2 . . . . . . . : '%s'\n", server->nick2);
    weechat_log_printf ("  nick3 . . . . . . . : '%s'\n", server->nick3);
    weechat_log_printf ("  username. . . . . . : '%s'\n", server->username);
    weechat_log_printf ("  realname. . . . . . : '%s'\n", server->realname);
    weechat_log_printf ("  command . . . . . . : '%s'\n",
                        (server->command && server->command[0]) ?
                        "(hidden)" : server->command);
    weechat_log_printf ("  command_delay . . . : %d\n",   server->command_delay);
    weechat_log_printf ("  autojoin. . . . . . : '%s'\n", server->autojoin);
    weechat_log_printf ("  autorejoin. . . . . : %d\n",   server->autorejoin);
    weechat_log_printf ("  notify_levels . . . : %s\n",   server->notify_levels);
    weechat_log_printf ("  child_pid . . . . . : %d\n",   server->child_pid);
    weechat_log_printf ("  child_read  . . . . : %d\n",   server->child_read);
    weechat_log_printf ("  child_write . . . . : %d\n",   server->child_write);
    weechat_log_printf ("  sock. . . . . . . . : %d\n",   server->sock);
    weechat_log_printf ("  is_connected. . . . : %d\n",   server->is_connected);
    weechat_log_printf ("  ssl_connected . . . : %d\n",   server->ssl_connected);
    weechat_log_printf ("  unterminated_message: '%s'\n", server->unterminated_message);
    weechat_log_printf ("  nick. . . . . . . . : '%s'\n", server->nick);
    weechat_log_printf ("  nick_modes. . . . . : '%s'\n", server->nick_modes);
    weechat_log_printf ("  prefix. . . . . . . : '%s'\n", server->prefix);
    weechat_log_printf ("  reconnect_start . . : %ld\n",  server->reconnect_start);
    weechat_log_printf ("  reconnect_join. . . : %d\n",   server->reconnect_join);
    weechat_log_printf ("  is_away . . . . . . : %d\n",   server->is_away);
    weechat_log_printf ("  away_message. . . . : '%s'\n", server->away_message);
    weechat_log_printf ("  away_time . . . . . : %ld\n",  server->away_time);
    weechat_log_printf ("  lag . . . . . . . . : %d\n",   server->lag);
    weechat_log_printf ("  lag_check_time. . . : tv_sec:%d, tv_usec:%d\n",
                        server->lag_check_time.tv_sec,
                        server->lag_check_time.tv_usec);
    weechat_log_printf ("  lag_next_check. . . : %ld\n",  server->lag_next_check);
    weechat_log_printf ("  last_user_message . : %ld\n",  server->last_user_message);
    weechat_log_printf ("  outqueue. . . . . . : 0x%X\n", server->outqueue);
    weechat_log_printf ("  last_outqueue . . . : 0x%X\n", server->last_outqueue);
    weechat_log_printf ("  buffer. . . . . . . : 0x%X\n", server->buffer);
    weechat_log_printf ("  channels. . . . . . : 0x%X\n", server->channels);
    weechat_log_printf ("  last_channel. . . . : 0x%X\n", server->last_channel);
    weechat_log_printf ("  prev_server . . . . : 0x%X\n", server->prev_server);
    weechat_log_printf ("  next_server . . . . : 0x%X\n", server->next_server);
}
