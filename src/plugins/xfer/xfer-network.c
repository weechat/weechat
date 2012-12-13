/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * xfer-network.c: network functions for xfer plugin
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include <netdb.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-chat.h"
#include "xfer-config.h"
#include "xfer-dcc.h"
#include "xfer-file.h"


/*
 * Creates pipe for communication with child process.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
xfer_network_create_pipe (struct t_xfer *xfer)
{
    int child_pipe[2];

    if (pipe (child_pipe) < 0)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to create pipe"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME);
        xfer_close (xfer, XFER_STATUS_FAILED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        return 0;
    }

    xfer->child_read = child_pipe[0];
    xfer->child_write = child_pipe[1];

    return 1;
}

/*
 * Writes data into pipe.
 */

void
xfer_network_write_pipe (struct t_xfer *xfer, int status, int error)
{
    char buffer[1 + 1 + 32 + 1];   /* status + error + pos + \0 */
    int num_written;

    snprintf (buffer, sizeof (buffer), "%c%c%032llu",
              status + '0', error + '0', xfer->pos);
    num_written = write (xfer->child_write, buffer, sizeof (buffer));
    (void) num_written;
}

/*
 * Reads data from child via pipe.
 */

int
xfer_network_child_read_cb (void *arg_xfer, int fd)
{
    struct t_xfer *xfer;
    char bufpipe[1 + 1 + 32 + 1];
    int num_read;

    /* make C compiler happy */
    (void) fd;

    xfer = (struct t_xfer *)arg_xfer;

    num_read = read (xfer->child_read, bufpipe, sizeof (bufpipe));
    if (num_read > 0)
    {
        sscanf (bufpipe + 2, "%llu", &xfer->pos);
        xfer->last_activity = time (NULL);
        xfer_file_calculate_speed (xfer, 0);

        /* read error code */
        switch (bufpipe[1] - '0')
        {
            /* errors for sender */
            case XFER_ERROR_READ_LOCAL:
                weechat_printf (NULL,
                                _("%s%s: unable to read local file"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
            case XFER_ERROR_SEND_BLOCK:
                weechat_printf (NULL,
                                _("%s%s: unable to send block to receiver"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
            case XFER_ERROR_READ_ACK:
                weechat_printf (NULL,
                                _("%s%s: unable to read ACK from receiver"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
            /* errors for receiver */
            case XFER_ERROR_CONNECT_SENDER:
                weechat_printf (NULL,
                                _("%s%s: unable to connect to sender"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
            case XFER_ERROR_RECV_BLOCK:
                weechat_printf (NULL,
                                _("%s%s: unable to receive block from sender"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
            case XFER_ERROR_WRITE_LOCAL:
                weechat_printf (NULL,
                                _("%s%s: unable to write local file"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
        }

        /* read new DCC status */
        switch (bufpipe[0] - '0')
        {
            case XFER_STATUS_ACTIVE:
                if (xfer->status == XFER_STATUS_CONNECTING)
                {
                    /* connection was successful by child, init transfer times */
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
 * Forks process for sending file.
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
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return;
            /* child process */
        case 0:
            setuid (getuid ());
            close (xfer->child_read);
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

    weechat_printf (NULL,
                    _("%s: sending file to %s (%ld.%ld.%ld.%ld, %s.%s), "
                      "name: %s (local filename: %s), %llu bytes (protocol: %s)"),
                    XFER_PLUGIN_NAME,
                    xfer->remote_nick,
                    xfer->remote_address >> 24,
                    (xfer->remote_address >> 16) & 0xff,
                    (xfer->remote_address >> 8) & 0xff,
                    xfer->remote_address & 0xff,
                    xfer->plugin_name,
                    xfer->plugin_id,
                    xfer->filename,
                    xfer->local_filename,
                    xfer->size,
                    xfer_protocol_string[xfer->protocol]);

    /* parent process */
    xfer->child_pid = pid;
    close (xfer->child_write);
    xfer->child_write = -1;
    xfer->hook_fd = weechat_hook_fd (xfer->child_read,
                                     1, 0, 0,
                                     &xfer_network_child_read_cb,
                                     xfer);
}

/*
 * Forks process for receiving file.
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
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return;
        /* child process */
        case 0:
            setuid (getuid ());
            close (xfer->child_read);
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
    close (xfer->child_write);
    xfer->child_write = -1;
    xfer->hook_fd = weechat_hook_fd (xfer->child_read,
                                     1, 0, 0,
                                     &xfer_network_child_read_cb,
                                     xfer);
}

/*
 * Kills child process and closes pipe.
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

/*
 * Callback called when data is available on xfer socket.
 */

int
xfer_network_fd_cb (void *arg_xfer, int fd)
{
    struct t_xfer *xfer;
    int sock, flags;
    struct sockaddr_in addr;
    socklen_t length;

    /* make C compiler happy */
    (void) fd;

    xfer = (struct t_xfer *)arg_xfer;

    if (xfer->status == XFER_STATUS_CONNECTING)
    {
        if (xfer->type == XFER_TYPE_FILE_SEND)
        {
            xfer->last_activity = time (NULL);
            length = sizeof (addr);
            sock = accept (xfer->sock,
                           (struct sockaddr *) &addr, &length);
            weechat_unhook (xfer->hook_fd);
            xfer->hook_fd = NULL;
            close (xfer->sock);
            xfer->sock = -1;
            if (sock < 0)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to create socket for sending "
                                  "file"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                return WEECHAT_RC_OK;
            }
            xfer->sock = sock;
            flags = fcntl (xfer->sock, F_GETFL);
            if (flags == -1)
                flags = 0;
            if (fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to set option \"nonblock\" "
                                  "for socket"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                return WEECHAT_RC_OK;
            }
            xfer->remote_address = ntohl (addr.sin_addr.s_addr);
            xfer->status = XFER_STATUS_ACTIVE;
            xfer->start_transfer = time (NULL);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            xfer_network_send_file_fork (xfer);
        }
    }

    if (xfer->status == XFER_STATUS_WAITING)
    {
        if (xfer->type == XFER_TYPE_CHAT_SEND)
        {
            length = sizeof (addr);
            sock = accept (xfer->sock, (struct sockaddr *) &addr, &length);
            weechat_unhook (xfer->hook_fd);
            xfer->hook_fd = NULL;
            close (xfer->sock);
            xfer->sock = -1;
            if (sock < 0)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to create socket for sending "
                                  "file"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                return WEECHAT_RC_OK;
            }
            xfer->sock = sock;
            flags = fcntl (xfer->sock, F_GETFL);
            if (flags == -1)
                flags = 0;
            if (fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to set option \"nonblock\" "
                                  "for socket"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                return WEECHAT_RC_OK;
            }
            xfer->remote_address = ntohl (addr.sin_addr.s_addr);
            xfer->status = XFER_STATUS_ACTIVE;
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            xfer->hook_fd = weechat_hook_fd (xfer->sock,
                                             1, 0, 0,
                                             &xfer_chat_recv_cb,
                                             xfer);
            xfer_chat_open_buffer (xfer);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called to check if there's a timeout for xfer (called only one time
 * for xfer).
 */

int
xfer_network_timer_cb (void *arg_xfer, int remaining_calls)
{
    struct t_xfer *xfer;

    /* make C compiler happy */
    (void) remaining_calls;

    xfer = (struct t_xfer *)arg_xfer;

    if ((xfer->status == XFER_STATUS_WAITING)
        || (xfer->status == XFER_STATUS_CONNECTING))
    {
        weechat_printf (NULL,
                        _("%s%s: timeout for \"%s\" with %s"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        xfer->filename, xfer->remote_nick);
        xfer_close (xfer, XFER_STATUS_FAILED);
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }

    return WEECHAT_RC_OK;
}

/*
 * Connects to another host.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
xfer_network_connect (struct t_xfer *xfer)
{
    int flags;

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

    if (XFER_IS_SEND(xfer->type))
    {
        /* listen to socket */
        flags = fcntl (xfer->sock, F_GETFL);
        if (flags == -1)
            flags = 0;
        if (fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK) == -1)
            return 0;
        if (listen (xfer->sock, 1) == -1)
            return 0;
        if (fcntl (xfer->sock, F_SETFL, flags) == -1)
            return 0;

        xfer->hook_fd = weechat_hook_fd (xfer->sock,
                                         1, 0, 0,
                                         &xfer_network_fd_cb,
                                         xfer);

        /* add timeout */
        if (weechat_config_integer (xfer_config_network_timeout) > 0)
        {
            xfer->hook_timer = weechat_hook_timer (weechat_config_integer (xfer_config_network_timeout) * 1000,
                                                   0, 1,
                                                   &xfer_network_timer_cb,
                                                   xfer);
        }
    }

    /* for chat receiving, connect to listening host */
    if (xfer->type == XFER_TYPE_CHAT_RECV)
    {
        flags = fcntl (xfer->sock, F_GETFL);
        if (flags == -1)
            flags = 0;
        if (fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK) == -1)
            return 0;
        weechat_network_connect_to (xfer->proxy, xfer->sock,
                                    xfer->remote_address, xfer->port);

        xfer->hook_fd = weechat_hook_fd (xfer->sock,
                                         1, 0, 0,
                                         &xfer_chat_recv_cb,
                                         xfer);
    }

    /* for file receiving, connection is made in child process (blocking) */

    return 1;
}

/*
 * Connects to sender and initializes file or chat.
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
            xfer_chat_open_buffer (xfer);
        }
    }
    xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
}

/*
 * Accepts a xfer file or chat request.
 */

void
xfer_network_accept (struct t_xfer *xfer)
{
    if (XFER_IS_FILE(xfer->type) && (xfer->start_resume > 0))
    {
        xfer->status = XFER_STATUS_CONNECTING;
        xfer_send_signal (xfer, "xfer_resume_ready");
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }
    else
        xfer_network_connect_init (xfer);
}
