/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../weechat.h"
#include "irc.h"
#include "../gui/gui.h"


t_irc_server *irc_servers = NULL;
t_irc_server *last_irc_server = NULL;
t_irc_server *current_irc_server = NULL;

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
    server->address = NULL;
    server->port = -1;
    server->password = NULL;
    server->nick1 = NULL;
    server->nick2 = NULL;
    server->nick3 = NULL;
    server->username = NULL;
    server->realname = NULL;
    server->nick = NULL;
    server->is_connected = 0;
    server->sock4 = -1;
    server->is_away = 0;
    server->server_read = -1;
    server->server_write = -1;
    server->window = NULL;
    server->channels = NULL;
    server->last_channel = NULL;
}

/*
 * server_alloc: allocate a new server and add it to the servers queue
 */

t_irc_server *
server_alloc ()
{
    t_irc_server *new_server;

    #if DEBUG >= 1
    log_printf ("allocating new server\n");
    #endif
    
    /* alloc memory for new server */
    if ((new_server = (t_irc_server *) malloc (sizeof (t_irc_server))) == NULL)
    {
        fprintf (stderr, _("%s cannot allocate new server"), WEECHAT_ERROR);
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
 * server_create_window: create windows for a server
 */

void
server_create_window (t_irc_server *server)
{
    if (!SERVER(gui_windows))
    {
        server->window = gui_windows;
        SERVER(gui_windows) = server;
    }
    else
        gui_window_new (server, NULL);
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
server_new (char *name, int autoconnect, char *address, int port,
            char *password, char *nick1, char *nick2, char *nick3,
            char *username, char *realname)
{
    t_irc_server *new_server;
    
    if (!name || !address || (port < 0))
        return NULL;
    
    #if DEBUG >= 1
    log_printf ("creating new server (name:%s, address:%s, port:%d, pwd:%s, "
                "nick1:%s, nick2:%s, nick3:%s, username:%s, realname:%s)\n",
                name, address, port, (password) ? password : "",
                (nick1) ? nick1 : "", (nick2) ? nick2 : "", (nick3) ? nick3 : "",
                (username) ? username : "", (realname) ? realname : "");
    #endif
    
    if ((new_server = server_alloc ()))
    {
        new_server->name = strdup (name);
        new_server->autoconnect = autoconnect;
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

int
server_sendf (t_irc_server * server, char *fmt, ...)
{
    va_list args;
    static char buffer[1024];
    int size_buf;

    if (!server)
        return -1;

    va_start (args, fmt);
    size_buf = vsnprintf (buffer, sizeof (buffer) - 1, fmt, args);
    va_end (args);

    if ((size_buf == 0) || (strcmp (buffer, "\r\n") == 0))
        return 0;

    buffer[sizeof (buffer) - 1] = '\0';
    if ((size_buf < 0) || (size_buf > (int) (sizeof (buffer) - 1)))
        size_buf = strlen (buffer);
    buffer[size_buf - 2] = '\0';
    #if DEBUG >= 2
    gui_printf (server->window, "[DEBUG] Sending to server >>> %s\n", buffer);
    #endif
    buffer[size_buf - 2] = '\r';
    return server_send (server, buffer, size_buf);
}

/*
 * server_msgq_add_msg: add a message to received messages queue (at the end)
 */

void
server_msgq_add_msg (t_irc_server * server, char *msg)
{
    t_irc_message *message;

    message = (t_irc_message *) malloc (sizeof (t_irc_message));
    message->server = server;
    if (unterminated_message)
    {
        message->data = (char *) malloc (strlen (unterminated_message) +
                                         strlen (msg) + 1);
        strcpy (message->data, unterminated_message);
        strcat (message->data, msg);
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
                strcpy (unterminated_message, buffer);
                return;
            }
            gui_printf (server->window,
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
    /*char **argv;
    int argc;*/
    char *ptr_data, *pos, *pos2;
    char *host, *command, *args;

    /* TODO: optimize this function, parse only a few messages (for low CPU time!) */
    while (recv_msgq)
    {
        #if DEBUG >= 2
        gui_printf (gui_current_window, "[DEBUG] %s\n", recv_msgq->data);
        #endif

        ptr_data = recv_msgq->data;
        
        while (ptr_data[0] == ' ')
            ptr_data++;
        
        if (ptr_data)
        {
            #if DEBUG >= 2
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
            
            switch (irc_recv_command (recv_msgq->server, host, command, args))
            {
                case -1:
                    gui_printf (recv_msgq->server->window,
                                _("Command '%s' failed!\n"), command);
                    break;
                case -2:
                    gui_printf (recv_msgq->server->window,
                                _("No command to execute!\n"));
                    break;
                case -3:
                    gui_printf (recv_msgq->server->window,
                                _("Unknown command: cmd=%s, args=%s\n"),
                                command, args);
                    break;
            }
        }

        free (recv_msgq->data);
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
 * server_connect: connect to an irc server
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

    gui_printf (server->window,
                _(WEECHAT_NAME ": connecting to %s:%d...\n"),
                server->address, server->port);
    log_printf ("connecting to server %s:%d...\n",
                server->address, server->port);
    server->is_connected = 0;

    /* create pipe */
    if (pipe (server_pipe) < 0)
    {
        gui_printf (server->window,
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
        gui_printf (server->window,
                    _("%s cannot set socket option 'SO_REUSEADDR'\n"),
                    WEECHAT_ERROR);
    set = 1;
    if (setsockopt
        (server->sock4, SOL_SOCKET, SO_KEEPALIVE, (char *) &set,
         sizeof (set)) == -1)
        gui_printf (server->window,
                    _("%s cannot set socket option \"SO_KEEPALIVE\"\n"),
                    WEECHAT_ERROR);

    /* bind to hostname */
    ip4_hostent = gethostbyname (server->address);
    if (!ip4_hostent)
    {
        gui_printf (server->window,
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
        gui_printf (server->window,
                    WEECHAT_ERORR "server_connect: can't bind to hostname\n");
        return 0;
    } */
    ip_address = inet_ntoa (addr.sin_addr);
    if (!ip_address)
    {
        gui_printf (server->window,
                    _("%s IP address not found\n"), WEECHAT_ERROR);
        close (server->server_read);
        close (server->server_write);
        close (server->sock4);
        server->sock4 = -1;
        return 0;
    }

    /* connection to server */
    gui_printf (server->window,
                _(WEECHAT_NAME ": server IP is: %s\n"), ip_address);

    error = connect (server->sock4, (struct sockaddr *) &addr, sizeof (addr));
    if (error != 0)
    {
        gui_printf (server->window,
                    _("%s cannot connect to irc server\n"), WEECHAT_ERROR);
        close (server->server_read);
        close (server->server_write);
        close (server->sock4);
        server->sock4 = -1;
        return 0;
    }

    current_irc_server = server;
    return 1;
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
        /* write disconnection message on each channel/private window */
        for (ptr_channel = server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            irc_display_prefix (ptr_channel->window, PREFIX_INFO);
            gui_printf (ptr_channel->window, "Disconnected from server!\n");
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
