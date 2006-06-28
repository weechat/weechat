/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
#include "../common/weeconfig.h"
#include "../gui/gui.h"


t_irc_server *irc_servers = NULL;
t_irc_server *last_irc_server = NULL;

t_irc_message *recv_msgq, *msgq_last_msg;

int check_away = 0;

char *nick_modes = "aiwroOs";


/*
 * server_init: init server struct with default values
 */

void
server_init (t_irc_server *server)
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
    server->charset_decode_iso = NULL;
    server->charset_decode_utf = NULL;
    server->charset_encode = NULL;
    
    /* internal vars */
    server->child_pid = 0;
    server->child_read = -1;
    server->child_write = -1;
    server->sock = -1;
    server->is_connected = 0;
    server->ssl_connected = 0;
    server->unterminated_message = NULL;
    server->nick = NULL;
    server->nick_modes = (char *) malloc (NUM_NICK_MODES + 1);
    memset (server->nick_modes, ' ', NUM_NICK_MODES);
    server->nick_modes[NUM_NICK_MODES] = '\0';
    server->reconnect_start = 0;
    server->reconnect_join = 0;
    server->is_away = 0;
    server->away_time = 0;
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) + cfg_irc_lag_check;
    server->cmd_list_regexp = NULL;
    server->buffer = NULL;
    server->saved_buffer = NULL;
    server->channels = NULL;
    server->last_channel = NULL;
}

/*
 * server_init_with_url: init a server with url of this form:
 *                       irc://nick:pass@irc.toto.org:6667
 *                       returns:  0 = ok
 *                                -1 = invalid syntax
 */

int
server_init_with_url (char *irc_url, t_irc_server *server)
{
    char *url, *pos_server, *pos_channel, *pos, *pos2;
    int ipv6, ssl;
    struct passwd *my_passwd;
    
    server_init (server);
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
            fprintf (stderr, "%s: %s (%s).",
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
        if (string_is_channel (pos_channel))
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
 * server_alloc: allocate a new server and add it to the servers queue
 */

t_irc_server *
server_alloc ()
{
    t_irc_server *new_server;
    
    /* alloc memory for new server */
    if ((new_server = (t_irc_server *) malloc (sizeof (t_irc_server))) == NULL)
    {
        fprintf (stderr, _("%s cannot allocate new server\n"), WEECHAT_ERROR);
        return NULL;
    }

    /* initialize new server */
    server_init (new_server);

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
 * server_destroy: free server data (not struct himself)
 */

void
server_destroy (t_irc_server *server)
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
    if (server->charset_decode_iso)
        free (server->charset_decode_iso);
    if (server->charset_decode_utf)
        free (server->charset_decode_utf);
    if (server->charset_encode)
        free (server->charset_encode);
    if (server->unterminated_message)
        free (server->unterminated_message);
    if (server->nick)
        free (server->nick);
    if (server->nick_modes)
        free (server->nick_modes);
    if (server->channels)
        channel_free_all (server);
}

/*
 * server_free: free a server and remove it from servers queue
 */

void
server_free (t_irc_server *server)
{
    t_irc_server *new_irc_servers;

    if (!server)
        return;
    
    /* close any opened channel/private */
    while (server->channels)
        channel_free (server, server->channels);
    
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
    
    server_destroy (server);
    free (server);
    irc_servers = new_irc_servers;
}

/*
 * server_free_all: free all allocated servers
 */

void
server_free_all ()
{
    /* for each server in memory, remove it */
    while (irc_servers)
        server_free (irc_servers);
}

/*
 * server_new: creates a new server, and initialize it
 */

t_irc_server *
server_new (char *name, int autoconnect, int autoreconnect,
            int autoreconnect_delay, int command_line, char *address,
            int port, int ipv6, int ssl, char *password,
            char *nick1, char *nick2, char *nick3, char *username,
            char *realname, char *hostname, char *command, int command_delay,
            char *autojoin, int autorejoin, char *notify_levels,
            char *charset_decode_iso, char *charset_decode_utf,
            char *charset_encode)
{
    t_irc_server *new_server;
    
    if (!name || !address || (port < 0))
        return NULL;
    
#ifdef DEBUG
    weechat_log_printf ("Creating new server (name:%s, address:%s, port:%d, pwd:%s, "
                        "nick1:%s, nick2:%s, nick3:%s, username:%s, realname:%s, "
                        "hostname: %s, command:%s, autojoin:%s, autorejoin:%s, "
                        "notify_levels:%s, decode_iso:%s, decode_utf:%s, encode:%s)\n",
                        name, address, port, (password) ? password : "",
                        (nick1) ? nick1 : "", (nick2) ? nick2 : "", (nick3) ? nick3 : "",
                        (username) ? username : "", (realname) ? realname : "",
                        (hostname) ? hostname : "", (command) ? command : "",
                        (autojoin) ? autojoin : "", (autorejoin) ? "on" : "off",
                        (notify_levels) ? notify_levels : "",
                        (charset_decode_iso) ? charset_decode_iso : "",
                        (charset_decode_utf) ? charset_decode_utf : "",
                        (charset_encode) ? charset_encode : "");
#endif
    
    if ((new_server = server_alloc ()))
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
        new_server->charset_decode_iso =
            (charset_decode_iso) ? strdup (charset_decode_iso) : NULL;
        new_server->charset_decode_utf =
            (charset_decode_utf) ? strdup (charset_decode_utf) : NULL;
        new_server->charset_encode =
            (charset_encode) ? strdup (charset_encode) : NULL;
    }
    else
        return NULL;
    return new_server;
}

/*
 * server_get_charset_decode_iso: get decode iso value for server
 *                                if not found for server, look for global
 */

char *
server_get_charset_decode_iso (t_irc_server *server)
{
    char *pos, *result;
    int length;
    
    if (!server)
        return (cfg_look_charset_decode_iso) ?
            strdup (cfg_look_charset_decode_iso) : strdup ("");
    
    config_option_list_get_value (&(server->charset_decode_iso),
                                  "server", &pos, &length);
    if (pos && (length > 0))
    {
        result = strdup (pos);
        result[length] = '\0';
        return result;
    }
    
    return (cfg_look_charset_decode_iso) ?
        strdup (cfg_look_charset_decode_iso) : strdup ("");
}

/*
 * server_get_charset_decode_utf: get decode utf value for server
 *                                if not found for server, look for global
 */

char *
server_get_charset_decode_utf (t_irc_server *server)
{
    char *pos, *result;
    int length;
    
    if (!server)
        return (cfg_look_charset_decode_utf) ?
            strdup (cfg_look_charset_decode_utf) : strdup ("");
    
    config_option_list_get_value (&(server->charset_decode_utf),
                                  "server", &pos, &length);
    if (pos && (length > 0))
    {
        result = strdup (pos);
        result[length] = '\0';
        return result;
    }
    
    return (cfg_look_charset_decode_utf) ?
        strdup (cfg_look_charset_decode_utf) : strdup ("");
}

/*
 * server_get_charset_encode: get encode value for server
 *                            if not found for server, look for global
 */

char *
server_get_charset_encode (t_irc_server *server)
{
    char *pos, *result;
    int length;
    
    if (!server)
        return (cfg_look_charset_encode) ?
            strdup (cfg_look_charset_encode) : strdup ("");
    
    config_option_list_get_value (&(server->charset_encode),
                                  "server", &pos, &length);
    if (pos && (length > 0))
    {
        result = strdup (pos);
        result[length] = '\0';
        return result;
    }
    
    return (cfg_look_charset_encode) ?
        strdup (cfg_look_charset_encode) : strdup ("");
}

/*
 * server_send: send data to IRC server
 */

int
server_send (t_irc_server *server, char *buffer, int size_buf)
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
 * server_sendf: send formatted data to IRC server
 */

void
server_sendf (t_irc_server *server, char *fmt, ...)
{
    va_list args;
    static char buffer[4096];
    int size_buf;
    
    if (!server)
        return;
    
    va_start (args, fmt);
    size_buf = vsnprintf (buffer, sizeof (buffer) - 1, fmt, args);
    va_end (args);

    if ((size_buf == 0) || (strcmp (buffer, "\r\n") == 0))
        return;
    
    buffer[sizeof (buffer) - 1] = '\0';
    if ((size_buf < 0) || (size_buf > (int) (sizeof (buffer) - 1)))
        size_buf = strlen (buffer);
    
    buffer[size_buf - 2] = '\0';
    gui_printf_raw_data (server, 1, buffer);
#ifdef DEBUG
    gui_printf (server->buffer, "[DEBUG] Sending to server >>> %s\n", buffer);
#endif
    buffer[size_buf - 2] = '\r';
    
    if (server_send (server, buffer, strlen (buffer)) <= 0)
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer, _("%s error sending data to IRC server\n"),
                    WEECHAT_ERROR);
    }
}

/*
 * server_msgq_add_msg: add a message to received messages queue (at the end)
 */

void
server_msgq_add_msg (t_irc_server *server, char *msg)
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
 * server_msgq_add_unterminated: add an unterminated message to queue
 */

void
server_msgq_add_unterminated (t_irc_server *server, char *string)
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
 * server_msgq_add_buffer: explode received buffer, creating queued messages
 */

void
server_msgq_add_buffer (t_irc_server *server, char *buffer)
{
    char *pos_cr, *pos_lf;

    while (buffer[0])
    {
        pos_cr = strchr (buffer, '\r');
        pos_lf = strchr (buffer, '\n');
        
        if (!pos_cr && !pos_lf)
        {
            /* no CR/LF found => add to unterminated and return */
            server_msgq_add_unterminated (server, buffer);
            return;
        }
        
        if (pos_cr && ((!pos_lf) || (pos_lf > pos_cr)))
        {
            /* found '\r' first => ignore this char */
            pos_cr[0] = '\0';
            server_msgq_add_unterminated (server, buffer);
            buffer = pos_cr + 1;
        }
        else
        {
            /* found: '\n' first => terminate message */
            pos_lf[0] = '\0';
            server_msgq_add_msg (server, buffer);
            buffer = pos_lf + 1;
        }
    }
}

/*
 * server_msgq_flush: flush message queue
 */

void
server_msgq_flush ()
{
    t_irc_message *next;
    char *entire_line, *ptr_data, *pos, *pos2;
    char *host, *command, *args;
    
    while (recv_msgq)
    {
        if (recv_msgq->data)
        {
#ifdef DEBUG
            gui_printf (gui_current_window->buffer, "[DEBUG] %s\n", recv_msgq->data);
#endif
            
            ptr_data = recv_msgq->data;
            entire_line = strdup (ptr_data);
            
            while (ptr_data[0] == ' ')
                ptr_data++;
            
            if (ptr_data && ptr_data[0])
            {
                gui_printf_raw_data (recv_msgq->server, 0, ptr_data);
#ifdef DEBUG
                gui_printf (NULL, "[DEBUG] data received from server: %s\n", ptr_data);
#endif
                
                host = NULL;
                command = NULL;
                args = ptr_data;
                
                if (ptr_data[0] == ':')
                {
                    pos = strchr (ptr_data, ' ');
                    if (pos)
                    {
                        pos[0] = '\0';
                        host = ptr_data + 1;
                        pos++;
                    }
                    else
                        pos = ptr_data;
                }
                else
                    pos = ptr_data;
                
                if (pos && pos[0])
                {
                    while (pos[0] == ' ')
                        pos++;
                    pos2 = strchr (pos, ' ');
                    if (pos2)
                    {
                        pos2[0] = '\0';
                        command = strdup (pos);
                        pos2++;
                        while (pos2[0] == ' ')
                            pos2++;
                        args = (pos2[0] == ':') ? pos2 + 1 : pos2;
                    }
                }
                
                switch (irc_recv_command (recv_msgq->server, entire_line, host,
                                          command, args))
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
                
                if (command)
                    free (command);
            }
            
            free (entire_line);
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
 * server_recv: receive data from an irc server
 */

void
server_recv (t_irc_server *server)
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
        server_msgq_add_buffer (server, buffer);
        server_msgq_flush ();
    }
    else
    {
        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
        gui_printf (server->buffer,
                    _("%s cannot read data from socket, disconnecting from server...\n"),
                    WEECHAT_ERROR);
        server_disconnect (server, 1);
    }
}

/*
 * server_kill_child: kill child process and close pipe
 */

void
server_kill_child (t_irc_server *server)
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
 * server_close_connection: close server connection (kill child, close socket/pipes)
 */

void
server_close_connection (t_irc_server *server)
{
    server_kill_child (server);
    
    /* close network socket */
    if (server->sock != -1)
    {
#ifdef HAVE_GNUTLS
        if (server->ssl_connected)
            gnutls_bye (server->gnutls_sess, GNUTLS_SHUT_RDWR);
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
    
    /* server is now disconnected */
    server->is_connected = 0;
    server->ssl_connected = 0;
}

/*
 * server_reconnect_schedule: schedule reconnect for a server
 */

void
server_reconnect_schedule (t_irc_server *server)
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
 * server_child_read: read connection progress from child process
 */

void
server_child_read (t_irc_server *server)
{
    char buffer[1];
    int num_read;
    
    num_read = read (server->child_read, buffer, sizeof (buffer));
    if (num_read != -1)
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
                                              (gnutls_transport_ptr) server->sock);
                    if (gnutls_handshake (server->gnutls_sess) < 0)
                    {
                        irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                        gui_printf (server->buffer,
                                    _("%s gnutls handshake failed\n"),
                                    WEECHAT_ERROR);
                        server_close_connection (server);
                        server_reconnect_schedule (server);
                        return;
                    }
                }
#endif
                /* kill child and login to server */
                server_kill_child (server);
                irc_login (server);
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
                server_close_connection (server);
                server_reconnect_schedule (server);
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
                server_close_connection (server);
                server_reconnect_schedule (server);
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
                server_close_connection (server);
                server_reconnect_schedule (server);
                break;
            /* proxy fails to connect to server */
            case '4':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf (server->buffer,
                            _("%s proxy fails to establish connection to "
                              "server (check username/password if used)\n"),
                            WEECHAT_ERROR);
                server_close_connection (server);
                server_reconnect_schedule (server);
                break;
            /* fails to set local hostname/IP */
            case '5':
                irc_display_prefix (server, server->buffer, PREFIX_ERROR);
                gui_printf (server->buffer,
                            _("%s unable to set local hostname/IP\n"),
                            WEECHAT_ERROR);
                server_close_connection (server);
                server_reconnect_schedule (server);
                break;
        }
    }
}

/*
 * convbase64_8x3_to_6x4 : convert 3 bytes of 8 bits in 4 bytes of 6 bits
 */

void
convbase64_8x3_to_6x4(char *from, char* to)
{

    unsigned char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    to[0] = base64_table [ (from[0] & 0xfc) >> 2 ];
    to[1] = base64_table [ ((from[0] & 0x03) << 4) + ((from[1] & 0xf0) >> 4) ];
    to[2] = base64_table [ ((from[1] & 0x0f) << 2) + ((from[2] & 0xc0) >> 6) ];
    to[3] = base64_table [ from[2] & 0x3f ];
}

/*
 * base64encode: encode a string in base64
 */

void
base64encode(char *from, char *to)
{

    char *f, *t;
    int from_len;

    from_len = strlen(from);

    f = from;
    t = to;
  
    while(from_len >= 3)
    {
        convbase64_8x3_to_6x4(f, t);
        f += 3 * sizeof(*f);
        t += 4 * sizeof(*t);
        from_len -= 3;
    }

    if (from_len > 0)
    {
        char rest[3] = { 0, 0, 0 };
        switch(from_len)
        {
        case 1 :
            rest[0] = f[0];
            convbase64_8x3_to_6x4(rest, t);
            t[2] = t[3] = '=';
            break;
        case 2 :
            rest[0] = f[0];
            rest[1] = f[1];
            convbase64_8x3_to_6x4(rest, t);
            t[3] = '=';
            break;
        }
        t[4] = 0;
    }
}

/*
 * pass_httpproxy: establish connection/authentification to an http proxy
 *                 return : 
 *                  - 0 if connexion throw proxy was successful
 *                  - 1 if connexion fails
 */

int
pass_httpproxy(int sock, char *address, int port)
{

    char buffer[256];
    char authbuf[128];
    char authbuf_base64[196];
    int n, m;

    if (strlen(cfg_proxy_username) > 0) 
    {
        /* authentification */
        snprintf(authbuf, sizeof(authbuf), "%s:%s", cfg_proxy_username, cfg_proxy_password);
        base64encode(authbuf, authbuf_base64);
        n = snprintf(buffer, sizeof(buffer), "CONNECT %s:%d HTTP/1.0\r\nProxy-Authorization: Basic %s\r\n\r\n", address, port, authbuf_base64);
    }
    else 
    {
        /* no authentification */
        n = snprintf(buffer, sizeof(buffer), "CONNECT %s:%d HTTP/1.0\r\n\r\n", address, port);
    }
  
    m = send (sock, buffer, n, 0);
    if (n != m)
        return 1;

    n = recv(sock, buffer, sizeof(buffer), 0);

    /* success result must be like: "HTTP/1.0 200 OK" */
    if (n < 12)
        return 1;

    if (memcmp (buffer, "HTTP/", 5) || memcmp (buffer + 9, "200", 3))
        return 1;
  
    return 0;
}

/*
 * resolve: resolve hostname on its IP address
 *          (works with ipv4 and ipv6)
 *          return :
 *           - 0 if resolution was successful
 *           - 1 if resolution fails
 */

int
resolve (char *hostname, char *ip, int *version)
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
 * pass_socks4proxy: establish connection/authentification throw a socks4 proxy
 *                   return : 
 *                    - 0 if connexion throw proxy was successful
 *                    - 1 if connexion fails
 */

int
pass_socks4proxy (int sock, char *address, int port, char *username)
{
    /* 
     * socks4 protocol is explain here: 
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
    resolve(address, ip_addr, NULL);
    socks4.address = inet_addr (ip_addr);
    strncpy (socks4.user, username, sizeof(socks4.user) - 1);
  
    send (sock, (char *) &socks4, 8 + strlen(socks4.user) + 1, 0);
    recv (sock, buffer, sizeof(buffer), 0);

    if (buffer[0] == 0 && buffer[1] == 90)
        return 0;

    return 1;
}

/*
 * pass_socks5proxy: establish connection/authentification throw a socks5 proxy
 *                   return : 
 *                    - 0 if connexion throw proxy was successful
 *                    - 1 if connexion fails
 */

int
pass_socks5proxy(int sock, char *address, int port)
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
  
    if (strlen(cfg_proxy_username) > 0)
        /* with authentication */
        socks5.method = 2;
    else
        /* without authentication */
        socks5.method = 0;
     
    send (sock, (char *) &socks5, sizeof(socks5), 0);
    /* server socks5 must respond with 2 bytes */
    if (recv (sock, buffer, 2, 0) != 2)
        return 1;
  
    if (strlen(cfg_proxy_username) > 0)
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
        memcpy(buffer + 3 + username_len, cfg_proxy_password, password_len);
        
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
    addr_buffer = (unsigned char *) malloc ( addr_buffer_len * sizeof(*addr_buffer));
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
    free(addr_buffer);

    /* dialog with proxy server */
    if (recv (sock, buffer, 4, 0) != 4)
        return 1;

    if (!(buffer[0] == 5 && buffer[1] == 0))
        return 1;

    switch(buffer[3]) {
        /* buffer[3] = address type */
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
 * pass_proxy: establish connection/authentification to a proxy
 *             return : 
 *              - 0 if connexion throw proxy was successful
 *              - 1 if connexion fails
 */

int
pass_proxy (int sock, char *address, int port, char *username)
{  
    if (strcmp (cfg_proxy_type_values[cfg_proxy_type], "http") == 0)
        return pass_httpproxy(sock, address, port);
    if (strcmp (cfg_proxy_type_values[cfg_proxy_type], "socks4") == 0)
        return pass_socks4proxy(sock, address, port, username);
    if (strcmp (cfg_proxy_type_values[cfg_proxy_type], "socks5") == 0)
        return pass_socks5proxy(sock, address, port);

    return 1;
}

/*
 * server_child: child process trying to connect to server
 */

int
server_child (t_irc_server *server)
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
        
        if (pass_proxy(server->sock, server->address, server->port, server->username))
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
 * server_connect: connect to an IRC server
 */

int
server_connect (t_irc_server *server)
{
    int child_pipe[2], set;
    pid_t pid;
#ifdef HAVE_GNUTLS
    const int cert_type_prio[] = { GNUTLS_CRT_X509, GNUTLS_CRT_OPENPGP, 0 };
#endif
    
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
    server_close_connection (server);
    
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
        gnutls_certificate_type_set_priority (server->gnutls_sess, cert_type_prio);
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
            server_close_connection (server);
            return 0;
        /* child process */
        case 0:
            setuid (getuid ());
            server_child (server);
            _exit (EXIT_SUCCESS);
    }
    
    /* parent process go on here */
    server->child_pid = pid;
    
    return 1;
}

/*
 * server_reconnect: reconnect to a server (after disconnection)
 */

void
server_reconnect (t_irc_server *server)
{
    irc_display_prefix (server, server->buffer, PREFIX_INFO);
    gui_printf (server->buffer, _("%s: Reconnecting to server...\n"),
                PACKAGE_NAME);
    server->reconnect_start = 0;
    
    if (server_connect (server))
        server->reconnect_join = 1;
    else
        server_reconnect_schedule (server);
}

/*
 * server_auto_connect: auto-connect to servers (called at startup)
 */

void
server_auto_connect (int auto_connect, int command_line)
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
            if (!server_connect (ptr_server))
                server_reconnect_schedule (ptr_server);
        }
    }
}

/*
 * server_disconnect: disconnect from an irc server
 */

void
server_disconnect (t_irc_server *server, int reconnect)
{
    t_irc_channel *ptr_channel;
    
    if (server->is_connected)
    {
        /* write disconnection message on each channel/private buffer */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            nick_free_all (ptr_channel);
            irc_display_prefix (NULL, ptr_channel->buffer, PREFIX_INFO);
            gui_printf (ptr_channel->buffer, _("Disconnected from server!\n"));
        }
    }
    
    server_close_connection (server);
    
    if (server->buffer)
    {
        irc_display_prefix (server, server->buffer, PREFIX_INFO);
        gui_printf (server->buffer, _("Disconnected from server!\n"));
    }
    
    if (server->nick_modes)
    {
        memset (server->nick_modes, ' ', NUM_NICK_MODES);
        server->nick_modes[NUM_NICK_MODES] = '\0';
    }
    server->is_away = 0;
    server->away_time = 0;
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) + cfg_irc_lag_check;
    
    if ((reconnect) && (server->autoreconnect))
        server_reconnect_schedule (server);
    else
        server->reconnect_start = 0;
    
    gui_window_redraw_buffer (gui_current_window->buffer);
}

/*
 * server_disconnect_all: disconnect from all irc servers
 */

void
server_disconnect_all ()
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        server_disconnect (ptr_server, 0);
}

/*
 * server_search: return pointer on a server with a name
 */

t_irc_server *
server_search (char *servername)
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
 * server_get_number_connected: returns number of connected server
 */

int
server_get_number_connected ()
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
 * server_get_number_buffer: returns position of a server and total number of
 *                           buffers with a buffer
 */

void
server_get_number_buffer (t_irc_server *server,
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
 * server_name_already_exists: return 1 if server name already exists
 *                             otherwise return 0
 */

int
server_name_already_exists (char *name)
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
 * server_remove_away: remove away for all chans/nicks (for all servers)
 */

void
server_remove_away ()
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
                    channel_remove_away (ptr_channel);
            }
        }
    }
}

/*
 * server_check_away: check for away on all channels (for all servers)
 */

void
server_check_away ()
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
                    channel_check_away (ptr_server, ptr_channel, 0);
            }
        }
    }
}

/*
 * server_set_away: set/unset away status for a server (all channels)
 */

void
server_set_away (t_irc_server *server, char *nick, int is_away)
{
    t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel; ptr_channel = ptr_channel->next_channel)
    {
        if (server->is_connected)
        {
            if (ptr_channel->type == CHANNEL_TYPE_CHANNEL)
                channel_set_away (ptr_channel, nick, is_away);
        }
    }
}

/*
 * server_print_log: print server infos in log (usually for crash dump)
 */

void
server_print_log (t_irc_server *server)
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
    weechat_log_printf ("  charset_decode_iso. : %s\n",   server->charset_decode_iso);
    weechat_log_printf ("  charset_decode_utf. : %s\n",   server->charset_decode_utf);
    weechat_log_printf ("  charset_encode. . . : %s\n",   server->charset_encode);
    weechat_log_printf ("  child_pid . . . . . : %d\n",   server->child_pid);
    weechat_log_printf ("  child_read  . . . . : %d\n",   server->child_read);
    weechat_log_printf ("  child_write . . . . : %d\n",   server->child_write);
    weechat_log_printf ("  sock. . . . . . . . : %d\n",   server->sock);
    weechat_log_printf ("  is_connected. . . . : %d\n",   server->is_connected);
    weechat_log_printf ("  ssl_connected . . . : %d\n",   server->ssl_connected);
    weechat_log_printf ("  unterminated_message: '%s'\n", server->unterminated_message);
    weechat_log_printf ("  nick. . . . . . . . : '%s'\n", server->nick);
    weechat_log_printf ("  nick_modes. . . . . : '%s'\n", server->nick_modes);
    weechat_log_printf ("  reconnect_start . . : %ld\n",  server->reconnect_start);
    weechat_log_printf ("  reconnect_join. . . : %d\n",   server->reconnect_join);
    weechat_log_printf ("  is_away . . . . . . : %d\n",   server->is_away);
    weechat_log_printf ("  away_time . . . . . : %ld\n",  server->away_time);
    weechat_log_printf ("  lag . . . . . . . . : %d\n",   server->lag);
    weechat_log_printf ("  lag_check_time. . . : tv_sec:%d, tv_usec:%d\n",
                        server->lag_check_time.tv_sec,
                        server->lag_check_time.tv_usec);
    weechat_log_printf ("  lag_next_check. . . : %ld\n",  server->lag_next_check);
    weechat_log_printf ("  buffer. . . . . . . : 0x%X\n", server->buffer);
    weechat_log_printf ("  channels. . . . . . : 0x%X\n", server->channels);
    weechat_log_printf ("  last_channel. . . . : 0x%X\n", server->last_channel);
    weechat_log_printf ("  prev_server . . . . : 0x%X\n", server->prev_server);
    weechat_log_printf ("  next_server . . . . : 0x%X\n", server->next_server);
}
