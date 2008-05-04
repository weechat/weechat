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

/* xfer-network.c: network functions for xfer plugin */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-network.h"
#include "xfer-buffer.h"
#include "xfer-dcc.h"
#include "xfer-file.h"


/*
 * xfer_network_connect: connect to another host
 */

int
xfer_network_connect (struct t_xfer *xfer)
{
    if (xfer->type == XFER_TYPE_CHAT_SEND)
        xfer->status = XFER_STATUS_WAITING;
    else
        xfer->status = XFER_STATUS_CONNECTING;
    
    if (xfer->sock < 0)
    {
        xfer->sock = socket (AF_INET, SOCK_STREAM, 0);
        if (xfer->sock < 0)
            return 0;
    }

    /* for chat or file sending, listen to socket for a connection */
    if (XFER_IS_SEND(xfer->type))
    {
        if (fcntl (xfer->sock, F_SETFL, O_NONBLOCK) == -1)
            return 0;	
        if (listen (xfer->sock, 1) == -1)
            return 0;
        if (fcntl (xfer->sock, F_SETFL, 0) == -1)
            return 0;
    }
    
    /* for chat receiving, connect to listening host */
    if (xfer->type == XFER_TYPE_CHAT_RECV)
    {
        if (fcntl (xfer->sock, F_SETFL, O_NONBLOCK) == -1)
            return 0;
        weechat_network_connect_to (xfer->sock, xfer->address, xfer->port);
    }

    /* for file receiving, connection is made in child process (blocking) */
    
    return 1;
}

/*
 * xfer_network_create_pipe: create pipe for communication with child process
 *                           return 1 if ok, 0 if error
 */

int
xfer_network_create_pipe (struct t_xfer *xfer)
{
    int child_pipe[2];
    
    if (pipe (child_pipe) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to create pipe"),
                        weechat_prefix ("error"), "xfer");
        xfer_close (xfer, XFER_STATUS_FAILED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        return 0;
    }
    
    xfer->child_read = child_pipe[0];
    xfer->child_write = child_pipe[1];
    
    return 1;
}

/*
 * xfer_network_write_pipe: write data into pipe
 */

void
xfer_network_write_pipe (struct t_xfer *xfer, int status, int error)
{
    char buffer[1 + 1 + 12 + 1];   /* status + error + pos + \0 */
    
    snprintf (buffer, sizeof (buffer), "%c%c%012lu",
              status + '0', error + '0', xfer->pos);
    write (xfer->child_write, buffer, sizeof (buffer));
}

/*
 * xfer_network_child_read_cb: read data from child via pipe
 */

int
xfer_network_child_read_cb (void *arg_xfer)
{
    struct t_xfer *xfer;
    char bufpipe[1 + 1 + 12 + 1];
    int num_read;
    char *error;
    
    xfer = (struct t_xfer *)arg_xfer;
    
    num_read = read (xfer->child_read, bufpipe, sizeof (bufpipe));
    if (num_read > 0)
    {
        error = NULL;
        xfer->pos = strtol (bufpipe + 2, &error, 10);
        xfer->last_activity = time (NULL);
        xfer_file_calculate_speed (xfer, 0);
        
        /* read error code */
        switch (bufpipe[1] - '0')
        {
            /* errors for sender */
            case XFER_ERROR_READ_LOCAL:
                weechat_printf (NULL,
                                _("%s%s: unable to read local file"),
                                weechat_prefix ("error"), "xfer");
                break;
            case XFER_ERROR_SEND_BLOCK:
                weechat_printf (NULL,
                                _("%s%s: unable to send block to receiver"),
                                weechat_prefix ("error"), "xfer");
                break;
            case XFER_ERROR_READ_ACK:
                weechat_printf (NULL,
                                _("%s%s: unable to read ACK from receiver"),
                                weechat_prefix ("error"), "xfer");
                break;
            /* errors for receiver */
            case XFER_ERROR_CONNECT_SENDER:
                weechat_printf (NULL,
                                _("%s%s: unable to connect to sender"),
                                weechat_prefix ("error"), "xfer");
                break;
            case XFER_ERROR_RECV_BLOCK:
                weechat_printf (NULL,
                                _("%s%s: unable to receive block from sender"),
                                weechat_prefix ("error"), "xfer");
                break;
            case XFER_ERROR_WRITE_LOCAL:
                weechat_printf (NULL,
                                _("%s%s: unable to write local file"),
                                weechat_prefix ("error"), "xfer");
                break;
        }
        
        /* read new DCC status */
        switch (bufpipe[0] - '0')
        {
            case XFER_STATUS_ACTIVE:
                if (xfer->status == XFER_STATUS_CONNECTING)
                {
                    /* connection was successful by child, init transfert times */
                    xfer->status = XFER_STATUS_ACTIVE;
                    xfer->start_transfer = time (NULL);
                    xfer->last_check_time = time (NULL);
                    xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                }
                else
                    xfer_buffer_refresh (WEECHAT_HOTLIST_LOW);
                break;
            case XFER_STATUS_DONE:
                xfer_close (xfer, XFER_STATUS_DONE);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                break;
            case XFER_STATUS_FAILED:
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                break;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_network_send_file_fork: fork process for sending file
 */

void
xfer_network_send_file_fork (struct t_xfer *xfer)
{
    pid_t pid;
    
    if (!xfer_network_create_pipe (xfer))
        return;
    
    xfer->file = open (xfer->local_filename, O_RDONLY | O_NONBLOCK, 0644);
    
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            weechat_printf (NULL,
                            _("%s%s: unable to fork"),
                            weechat_prefix ("error"), "xfer");
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return;
            /* child process */
        case 0:
            setuid (getuid ());
            switch (xfer->protocol)
            {
                case XFER_NO_PROTOCOL:
                    _exit (EXIT_SUCCESS);
                    break;
                case XFER_PROTOCOL_DCC:
                    xfer_dcc_send_file_child (xfer);
                    break;
                case XFER_NUM_PROTOCOLS:
                    break;
            }
            _exit (EXIT_SUCCESS);
    }
    
    /* parent process */
    xfer->child_pid = pid;
    xfer->hook_fd = weechat_hook_fd (xfer->child_read,
                                     1, 0, 0,
                                     xfer_network_child_read_cb,
                                     xfer);
}

/*
 * xfer_network_recv_file_fork: fork process for receiving file
 */

void
xfer_network_recv_file_fork (struct t_xfer *xfer)
{
    pid_t pid;
    
    if (!xfer_network_create_pipe (xfer))
        return;
    
    if (xfer->start_resume > 0)
        xfer->file = open (xfer->local_filename,
                           O_APPEND | O_WRONLY | O_NONBLOCK);
    else
        xfer->file = open (xfer->local_filename,
                           O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK,
                           0644);
    
    switch (pid = fork ())
    {
        /* fork failed */
        case -1:
            weechat_printf (NULL,
                            _("%s%s: unable to fork"),
                            weechat_prefix ("error"), "xfer");
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return;
            /* child process */
        case 0:
            setuid (getuid ());
            switch (xfer->protocol)
            {
                case XFER_NO_PROTOCOL:
                    _exit (EXIT_SUCCESS);
                    break;
                case XFER_PROTOCOL_DCC:
                    xfer_dcc_recv_file_child (xfer);
                    break;
                case XFER_NUM_PROTOCOLS:
                    break;
            }
            _exit (EXIT_SUCCESS);
    }
    
    /* parent process */
    xfer->child_pid = pid;
    xfer->hook_fd = weechat_hook_fd (xfer->child_read,
                                     1, 0, 0,
                                     xfer_network_child_read_cb,
                                     xfer);
}

/*
 * xfer_network_connect_init: connect to sender and init file or chat
 */

void
xfer_network_connect_init (struct t_xfer *xfer)
{
    if (!xfer_network_connect (xfer))
    {
        xfer_close (xfer, XFER_STATUS_FAILED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }
    else
    {
        /* for a file: launch child process */
        if (XFER_IS_FILE(xfer->type))
        {
            xfer->status = XFER_STATUS_CONNECTING;
            xfer_network_recv_file_fork (xfer);
        }
        else
        {
            /* for a chat => associate with buffer */
            xfer->status = XFER_STATUS_ACTIVE;
            // TODO: create buffer for xfer chat
        }
    }
    xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
}

/*
 * xfer_network_child_kill: kill child process and close pipe
 */

void
xfer_network_child_kill (struct t_xfer *xfer)
{
    /* kill process */
    if (xfer->child_pid > 0)
    {
        kill (xfer->child_pid, SIGKILL);
        waitpid (xfer->child_pid, NULL, 0);
        xfer->child_pid = 0;
    }
    
    /* close pipe used with child */
    if (xfer->child_read != -1)
    {
        close (xfer->child_read);
        xfer->child_read = -1;
    }
    if (xfer->child_write != -1)
    {
        close (xfer->child_write);
        xfer->child_write = -1;
    }
}
