/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* irc-server.c: (dis)connection and communication with irc server */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../common/weechat.h"
#include "irc.h"
#include "../gui/gui.h"


t_irc_server *irc_servers = NULL;
t_irc_server *last_irc_server = NULL;

t_irc_message *recv_msgq, *msgq_last_msg;

/* buffer containing beginning of message if not ending with \r\n */
char *unterminated_message = NULL;


/*
 * server_init: init server struct with default values
 */

void
server_init (t_irc_server *server)
{
    server->name = NULL;
    server->autoconnect = 0;
    server->command_line = 0;
    server->address = NULL;
    server->port = -1;
    server->password = NULL;
    server->nick1 = NULL;
    server->nick2 = NULL;
    server->nick3 = NULL;
    server->username = NULL;
    server->realname = NULL;
    server->command = NULL;
    server->autojoin = NULL;
    server->nick = NULL;
    server->is_connected = 0;
    server->sock4 = -1;
    server->is_away = 0;
    server->server_read = -1;
    server->server_write = -1;
    server->buffer = NULL;
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
    struct passwd *my_passwd;
    
    server_init (server);
    if (strncasecmp (irc_url, "irc://", 6) != 0)
        return -1;
    url = strdup (irc_url);
    pos_server = strchr (url, '@');
    if (pos_server)
    {
        pos_server[0] = '\0';
        pos_server++;
        pos = url + 6;
        if (!pos[0])
        {
            free (url);
            return -1;
        }
        pos2 = strchr (pos, ':');
        if (pos2)
        {
            pos2[0] = '\0';
            server->password = strdup (pos2 + 1);
        }
        server->nick1 = strdup (pos);
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
        pos_server = url + 6;
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
    if (server->command)
        free (server->command);
    if (server->autojoin)
        free (server->autojoin);
    if (server->nick)
        free (server->nick);
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
server_new (char *name, int autoconnect, int command_line, char *address,
            int port, char *password, char *nick1, char *nick2, char *nick3,
            char *username, char *realname, char *command, char *autojoin)
{
    t_irc_server *new_server;
    
    if (!name || !address || (port < 0))
        return NULL;
    
    #ifdef DEBUG
    wee_log_printf ("creating new server (name:%s, address:%s, port:%d, pwd:%s, "
                    "nick1:%s, nick2:%s, nick3:%s, username:%s, realname:%s, "
                    "command:%s, autojoin:%s)\n",
                    name, address, port, (password) ? password : "",
                    (nick1) ? nick1 : "", (nick2) ? nick2 : "", (nick3) ? nick3 : "",
                    (username) ? username : "", (realname) ? realname : "",
                    (command) ? command : "", (autojoin) ? autojoin : "");
    #endif
    
    if ((new_server = server_alloc ()))
    {
        new_server->name = strdup (name);
        new_server->autoconnect = autoconnect;
        new_server->command_line = command_line;
        new_server->address = strdup (address);
        new_server->port = port;
        new_server->password = (password) ? strdup (password) : strdup ("");
        new_server->nick1 = (nick1) ? strdup (nick1) : strdup ("weechat_user");
        new_server->nick2 = (nick2) ? strdup (nick2) : strdup ("weechat2");
        new_server->nick3 = (nick3) ? strdup (nick3) : strdup ("weechat3");
        new_server->username =
            (username) ? strdup (username) : strdup ("weechat");
        new_server->realname =
            (realname) ? strdup (realname) : strdup ("realname");
        new_server->command =
            (command) ? strdup (command) : NULL;
        new_server->autojoin =
            (autojoin) ? strdup (autojoin) : NULL;
        new_server->nick = strdup (new_server->nick1);
    }
    else
        return NULL;
    return new_server;
}

/*
 * server_send: send data to irc server
 */

int
server_send (t_irc_server * server, char *buffer, int size_buf)
{
    if (!server)
        return -1;
    
    return send (server->sock4, buffer, size_buf, 0);
}

/*
 * server_sendf: send formatted data to irc server
 */

void
server_sendf (t_irc_server * server, char *fmt, ...)
{
    va_list args;
    static char buffer[1024];
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
    #ifdef DEBUG
    gui_printf (server->buffer, "[DEBUG] Sending to server >>> %s\n", buffer);
    #endif
    buffer[size_buf - 2] = '\r';
    if (server_send (server, buffer, size_buf) <= 0)
        gui_printf (server->buffer, _("%s error sending data to IRC server\n"),
                    WEECHAT_ERROR);
}

/*
 * server_msgq_add_msg: add a message to received messages queue (at the end)
 */

void
server_msgq_add_msg (t_irc_server *server, char *msg)
{
    t_irc_message *message;

    message = (t_irc_message *) malloc (sizeof (t_irc_message));
    if (!message)
    {
        gui_printf (server->buffer,
                    _("%s not enough memory for received IRC message\n"),
                    WEECHAT_ERROR);
        return;
    }
    message->server = server;
    if (unterminated_message)
    {
        message->data = (char *) malloc (strlen (unterminated_message) +
                                         strlen (msg) + 1);
        if (!message->data)
            gui_printf (server->buffer,
                        _("%s not enough memory for received IRC message\n"),
                        WEECHAT_ERROR);
        else
        {
            strcpy (message->data, unterminated_message);
            strcat (message->data, msg);
        }
        free (unterminated_message);
        unterminated_message = NULL;
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
 * server_msgq_add_buffer: explode received buffer, creating queued messages
 */

void
server_msgq_add_buffer (t_irc_server * server, char *buffer)
{
    char *pos;

    while (buffer[0])
    {
        pos = strstr (buffer, "\r\n");
        if (pos)
        {
            pos[0] = '\0';
            server_msgq_add_msg (server, buffer);
            buffer = pos + 2;
        }
        else
        {
            pos = strchr (buffer, '\0');
            if (pos)
            {
                unterminated_message =
                    (char *) realloc (unterminated_message,
                                      strlen (buffer) + 1);
                if (!unterminated_message)
                    gui_printf (server->buffer,
                                _("%s not enough memory for received IRC message\n"),
                                WEECHAT_ERROR);
                else
                    strcpy (unterminated_message, buffer);
                return;
            }
            gui_printf (server->buffer,
                        _("%s unable to explode received buffer\n"),
                        WEECHAT_ERROR);
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

    /* TODO: optimize this function, parse only a few messages (for low CPU time!) */
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
            
            if (ptr_data)
            {
                #ifdef DEBUG
                gui_printf (NULL, "[DEBUG] data received from server: %s\n", ptr_data);
                #endif
                
                host = NULL;
                command = NULL;
                args = ptr_data;
                
                if (ptr_data[0] == ':')
                {
                    pos = strchr(ptr_data, ' ');
                    pos[0] = '\0';
                    host = ptr_data+1;
                    pos++;
                }
                else
                    pos = ptr_data;
                
                if (pos != NULL)
                {
                    while (pos[0] == ' ')
                        pos++;
                    pos2 = strchr(pos, ' ');
                    if (pos2 != NULL)
                    {
                        pos2[0] = '\0';
                        command = strdup(pos);
                        pos2++;
                        while (pos2[0] == ' ')
                            pos2++;
                        args = (pos2[0] == ':') ? pos2+1 : pos2;
                    }
                }
                
                switch (irc_recv_command (recv_msgq->server, entire_line, host,
                                          command, args))
                {
                    case -1:
                        gui_printf (recv_msgq->server->buffer,
                                    _("Command '%s' failed!\n"), command);
                        break;
                    case -2:
                        gui_printf (recv_msgq->server->buffer,
                                    _("No command to execute!\n"));
                        break;
                    case -3:
                        gui_printf (recv_msgq->server->buffer,
                                    _("Unknown command: cmd=%s, args=%s\n"),
                                    command, args);
                        break;
                }
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

    num_read = recv (server->sock4, buffer, sizeof (buffer) - 2, 0);
    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        server_msgq_add_buffer (server, buffer);
        server_msgq_flush ();
    }
}

/*
 * server_connect: connect to an IRC server
 */

int
server_connect (t_irc_server *server)
{
    int set;
    struct hostent *ip4_hostent;
    struct sockaddr_in addr;
    char *ip_address;
    int error;
    int server_pipe[2];

    gui_printf (server->buffer,
                _("%s: connecting to %s:%d...\n"),
                PACKAGE_NAME, server->address, server->port);
    wee_log_printf (_("connecting to server %s:%d...\n"),
                    server->address, server->port);
    server->is_connected = 0;

    /* create pipe */
    if (pipe (server_pipe) < 0)
    {
        gui_printf (server->buffer,
                    _("%s cannot create pipe\n"), WEECHAT_ERROR);
        server_free (server);
        return 0;
    }
    server->server_read = server_pipe[0];
    server->server_write = server_pipe[1];

    /* create socket and set options */
    server->sock4 = socket (AF_INET, SOCK_STREAM, 0);
    set = 1;
    if (setsockopt
        (server->sock4, SOL_SOCKET, SO_REUSEADDR, (char *) &set,
         sizeof (set)) == -1)
        gui_printf (server->buffer,
                    _("%s cannot set socket option \"SO_REUSEADDR\"\n"),
                    WEECHAT_ERROR);
    set = 1;
    if (setsockopt
        (server->sock4, SOL_SOCKET, SO_KEEPALIVE, (char *) &set,
         sizeof (set)) == -1)
        gui_printf (server->buffer,
                    _("%s cannot set socket option \"SO_KEEPALIVE\"\n"),
                    WEECHAT_ERROR);

    /* bind to hostname */
    ip4_hostent = gethostbyname (server->address);
    if (!ip4_hostent)
    {
        gui_printf (server->buffer,
                    _("%s address \"%s\" not found\n"),
                    WEECHAT_ERROR, server->address);
        close (server->server_read);
        close (server->server_write);
        close (server->sock4);
        server->sock4 = -1;
        return 0;
    }
    memset (&addr, 0, sizeof (addr));
    memcpy (&addr.sin_addr, ip4_hostent->h_addr, ip4_hostent->h_length);
    addr.sin_port = htons (server->port);
    addr.sin_family = AF_INET;
    /*error = bind(server->sock4, (struct sockaddr *)(&addr), sizeof(addr));
    if (error != 0)
    {
        gui_printf (server->buffer,
                    WEECHAT_ERORR "server_connect: can't bind to hostname\n");
        return 0;
    } */
    ip_address = inet_ntoa (addr.sin_addr);
    if (!ip_address)
    {
        gui_printf (server->buffer,
                    _("%s IP address not found\n"), WEECHAT_ERROR);
        close (server->server_read);
        close (server->server_write);
        close (server->sock4);
        server->sock4 = -1;
        return 0;
    }

    /* connection to server */
    gui_printf (server->buffer,
                _("%s: server IP is: %s\n"), PACKAGE_NAME, ip_address);

    error = connect (server->sock4, (struct sockaddr *) &addr, sizeof (addr));
    if (error != 0)
    {
        gui_printf (server->buffer,
                    _("%s cannot connect to irc server\n"), WEECHAT_ERROR);
        close (server->server_read);
        close (server->server_write);
        close (server->sock4);
        server->sock4 = -1;
        return 0;
    }
    
    return 1;
}

/*
 * server_auto_connect: auto-connect to servers (called at startup)
 */

void
server_auto_connect (int command_line)
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if ( ((command_line) && (ptr_server->command_line))
            || ((!command_line) && (ptr_server->autoconnect)) )
        {
            (void) gui_buffer_new (gui_current_window, ptr_server, NULL, 0, 1);
            if (server_connect (ptr_server))
                irc_login (ptr_server);
        }
    }
}

/*
 * server_disconnect: disconnect from an irc server
 */

void
server_disconnect (t_irc_server *server)
{
    t_irc_channel *ptr_channel;
    
    if (server->is_connected)
    {
        /* write disconnection message on each channel/private buffer */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_display_prefix (ptr_channel->buffer, PREFIX_INFO);
            gui_printf (ptr_channel->buffer, _("Disconnected from server!\n"));
        }
        
        /* close communication with server */
        close (server->server_read);
        close (server->server_write);
        close (server->sock4);
        server->is_connected = 0;
        server->sock4 = -1;
    }
}

/*
 * server_disconnect_all: disconnect from all irc servers
 */

void
server_disconnect_all ()
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
        server_disconnect (ptr_server);
}

/*
 * server_search: return pointer on a server with a name
 */

t_irc_server *
server_search (char *servername)
{
    t_irc_server *ptr_server;
    
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
 * server_name_already_exists: return 1 if server name already exists
 *                             otherwise return 0
 */

int
server_name_already_exists (char *name)
{
    t_irc_server *ptr_server;
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        if (strcmp (ptr_server->name, name) == 0)
            return 1;
    }
    return 0;
}
