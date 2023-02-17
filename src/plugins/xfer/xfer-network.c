/*
 * xfer-network.c - network functions for xfer plugin
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-chat.h"
#include "xfer-config.h"
#include "xfer-dcc.h"
#include "xfer-file.h"


/*
 * Converts integer address (as string) to IPv4 string using notation
 * "a.b.c.d".
 *
 * For example: "3232235778" -> "192.168.1.2"
 *
 * Note: result must be freed after use.
 */

char *
xfer_network_convert_integer_to_ipv4 (const char *str_address)
{
    char *error, result[128];
    long number;

    if (!str_address || !str_address[0])
        return NULL;

    number = strtol (str_address, &error, 10);
    if (!error || error[0] || (number <= 0))
        return NULL;

    snprintf (result, sizeof (result),
              "%ld.%ld.%ld.%ld",
              (number >> 24) & 0xFF,
              (number >> 16) & 0xFF,
              (number >> 8) & 0xFF,
              number & 0xFF);

    return strdup (result);
}

/*
 * Resolves address.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
xfer_network_resolve_addr (const char *str_address, const char *str_port,
                           struct sockaddr *addr, socklen_t *addr_len,
                           int ai_flags)
{
    struct addrinfo *ainfo, hints;
    char *converted_address;
    int rc;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_flags = ai_flags;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    res_init ();

    rc = getaddrinfo (str_address, str_port, &hints, &ainfo);

    /*
     * workaround for termux where an IP address as integer is not supported:
     * it returns an error EAI_NONAME (8); in this case we manually convert
     * the integer to IPv4 string, for example: 3232235778 -> 192.168.1.2
     */
    if (rc == EAI_NONAME)
    {
        converted_address = xfer_network_convert_integer_to_ipv4 (str_address);
        if (converted_address)
        {
            rc = getaddrinfo (converted_address, str_port, &hints, &ainfo);
            free (converted_address);
        }
    }

    if ((rc == 0) && ainfo && ainfo->ai_addr)
    {
        if (ainfo->ai_addrlen > *addr_len)
        {
            weechat_printf (NULL,
                            _("%s%s: address \"%s\" resolved to a larger "
                              "sockaddr than expected"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            str_address);
            freeaddrinfo (ainfo);
            return 0;
        }
        memcpy (addr, ainfo->ai_addr, ainfo->ai_addrlen);
        *addr_len = ainfo->ai_addrlen;
        freeaddrinfo (ainfo);
        return 1;
    }

    weechat_printf (NULL,
                    _("%s%s: invalid address \"%s\": error %d %s"),
                    weechat_prefix ("error"), XFER_PLUGIN_NAME,
                    str_address, rc, gai_strerror (rc));
    if ((rc == 0) && ainfo)
        freeaddrinfo (ainfo);
    return 0;
}

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
                        _("%s%s: unable to create pipe: error %d %s"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME,
                        errno, strerror (errno));
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
xfer_network_child_read_cb (const void *pointer, void *data, int fd)
{
    struct t_xfer *xfer;
    char bufpipe[1 + 1 + 32 + 1];
    int num_read;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    xfer = (struct t_xfer *)pointer;

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
            case XFER_ERROR_SEND_ACK:
                weechat_printf (NULL,
                                _("%s%s: unable to send ACK to sender"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                break;
            case XFER_ERROR_HASH_MISMATCH:
                weechat_printf (NULL,
                                _("%s%s: wrong CRC32 for file %s"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                xfer->filename);
                xfer->hash_status = XFER_HASH_STATUS_MISMATCH;
                break;
            case XFER_ERROR_HASH_RESUME_ERROR:
                weechat_printf (NULL,
                                _("%s%s: CRC32 error while resuming"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME);
                xfer->hash_status = XFER_HASH_STATUS_RESUME_ERROR;
                break;
        }

        /* read new DCC status */
        switch (bufpipe[0] - '0')
        {
            case XFER_STATUS_CONNECTING:
                xfer->status = XFER_STATUS_CONNECTING;
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                break;
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
            case XFER_STATUS_HASHING:
                xfer->status = XFER_STATUS_HASHING;
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                break;
            case XFER_STATUS_HASHED:
                if (bufpipe[1] - '0' == XFER_NO_ERROR)
                    xfer->hash_status = XFER_HASH_STATUS_MATCH;
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
    int rc;

    if (!xfer_network_create_pipe (xfer))
        return;

    xfer->file = open (xfer->local_filename, O_RDONLY | O_NONBLOCK, 0644);

    switch (pid = fork ())
    {
        case -1:  /* fork failed */
            weechat_printf (NULL,
                            _("%s%s: unable to fork (%s)"),
                            weechat_prefix ("error"),
                            XFER_PLUGIN_NAME,
                            strerror (errno));
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return;
        case 0:  /* child process */
            rc = setuid (getuid ());
            (void) rc;
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
                    _("%s: sending file to %s (%s, %s.%s), "
                      "name: %s (local filename: %s), %llu bytes (protocol: %s)"),
                    XFER_PLUGIN_NAME,
                    xfer->remote_nick,
                    xfer->remote_address_str,
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
                                     xfer, NULL);
}

/*
 * Forks process for receiving file.
 */

void
xfer_network_recv_file_fork (struct t_xfer *xfer)
{
    pid_t pid;
    int rc;

    if (!xfer_network_create_pipe (xfer))
        return;

    if (xfer->start_resume > 0)
    {
        xfer->file = open (xfer->temp_local_filename,
                           O_APPEND | O_WRONLY | O_NONBLOCK);
    }
    else
    {
        xfer->file = open (xfer->temp_local_filename,
                           O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK,
                           0644);
    }

    switch (pid = fork ())
    {
        case -1:  /* fork failed */
            weechat_printf (NULL,
                            _("%s%s: unable to fork (%s)"),
                            weechat_prefix ("error"),
                            XFER_PLUGIN_NAME,
                            strerror (errno));
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return;
        case 0:  /* child process */
            rc = setuid (getuid ());
            (void) rc;
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
                                     xfer, NULL);
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
xfer_network_fd_cb (const void *pointer, void *data, int fd)
{
    struct t_xfer *xfer;
    int sock, flags, error;
    struct sockaddr_storage addr;
    socklen_t length;
    char str_address[NI_MAXHOST];

    /* make C compiler happy */
    (void) data;
    (void) fd;

    length = sizeof (addr);
    memset (&addr, 0, length);
    xfer = (struct t_xfer *)pointer;

    if (xfer->status == XFER_STATUS_CONNECTING)
    {
        if (xfer->type == XFER_TYPE_FILE_SEND)
        {
            xfer->last_activity = time (NULL);
            sock = accept (xfer->sock,
                           (struct sockaddr *) &addr, &length);
            error = errno;
            weechat_unhook (xfer->hook_fd);
            xfer->hook_fd = NULL;
            close (xfer->sock);
            xfer->sock = -1;
            if (sock < 0)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to create socket for sending "
                                  "file: error %d %s"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                error, strerror (error));
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
                                  "for socket: error %d %s"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                errno, strerror (errno));
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                return WEECHAT_RC_OK;
            }
            error = getnameinfo ((struct sockaddr *)&addr, length, str_address,
                                 sizeof (str_address), NULL, 0, NI_NUMERICHOST);
            if (error != 0)
            {
                snprintf (str_address, sizeof (str_address),
                          "error: %s", gai_strerror (error));
            }
            xfer_set_remote_address (xfer, (struct sockaddr *)&addr, length,
                                     str_address);
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
            error = errno;
            weechat_unhook (xfer->hook_fd);
            xfer->hook_fd = NULL;
            close (xfer->sock);
            xfer->sock = -1;
            if (sock < 0)
            {
                weechat_printf (NULL,
                                _("%s%s: unable to create socket for sending "
                                  "file: error %d %s"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                error, strerror (error));
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
                                  "for socket: error %d %s"),
                                weechat_prefix ("error"), XFER_PLUGIN_NAME,
                                errno, strerror (errno));
                xfer_close (xfer, XFER_STATUS_FAILED);
                xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
                return WEECHAT_RC_OK;
            }
            error = getnameinfo ((struct sockaddr *)&addr, length, str_address,
                                 sizeof (str_address), NULL, 0, NI_NUMERICHOST);
            if (error != 0)
            {
                snprintf (str_address, sizeof (str_address),
                          "error: %s", gai_strerror (error));
            }
            xfer_set_remote_address (xfer, (struct sockaddr *)&addr, length,
                                     str_address);
            xfer->status = XFER_STATUS_ACTIVE;
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            xfer->hook_fd = weechat_hook_fd (xfer->sock,
                                             1, 0, 0,
                                             &xfer_chat_recv_cb,
                                             xfer, NULL);
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
xfer_network_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    struct t_xfer *xfer;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    xfer = (struct t_xfer *)pointer;

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
 * Callback called when connecting to remote host (DCC chat only).
 */

int
xfer_network_connect_chat_recv_cb (const void *pointer, void *data,
                                   int status, int gnutls_rc,
                                   int sock, const char *error,
                                   const char *ip_address)
{
    struct t_xfer *xfer;
    int flags;

    /* make C compiler happy */
    (void) data;
    (void) gnutls_rc;
    (void) ip_address;

    xfer = (struct t_xfer*)pointer;

    weechat_unhook (xfer->hook_connect);
    xfer->hook_connect = NULL;

    /* connection OK? */
    if (status == WEECHAT_HOOK_CONNECT_OK)
    {
        xfer->sock = sock;

        flags = fcntl (xfer->sock, F_GETFL);
        if (flags == -1)
            flags = 0;
        if (fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            weechat_printf (NULL,
                            _("%s%s: unable to set option \"nonblock\" "
                              "for socket: error %d %s"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            errno, strerror (errno));
            close (xfer->sock);
            xfer->sock = -1;
            xfer_close (xfer, XFER_STATUS_FAILED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
            return WEECHAT_RC_OK;
        }

        xfer->hook_fd = weechat_hook_fd (xfer->sock,
                                         1, 0, 0,
                                         &xfer_chat_recv_cb,
                                         xfer, NULL);

        xfer_chat_open_buffer (xfer);
        xfer->status = XFER_STATUS_ACTIVE;
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);

        return WEECHAT_RC_OK;
    }

    /* connection error */
    switch (status)
    {
        case WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND:
            weechat_printf (NULL,
                            (xfer->proxy && xfer->proxy[0]) ?
                            _("%s%s: proxy address \"%s\" not found") :
                            _("%s%s: address \"%s\" not found"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            xfer->remote_address_str);
            break;
        case WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND:
            weechat_printf (NULL,
                            (xfer->proxy && xfer->proxy[0]) ?
                            _("%s%s: proxy IP address not found") :
                            _("%s%s: IP address not found"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            break;
        case WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED:
            weechat_printf (NULL,
                            (xfer->proxy && xfer->proxy[0]) ?
                            _("%s%s: proxy connection refused") :
                            _("%s%s: connection refused"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            break;
        case WEECHAT_HOOK_CONNECT_PROXY_ERROR:
            weechat_printf (NULL,
                            _("%s%s: proxy fails to establish connection to "
                              "server (check username/password if used and if "
                              "server address/port is allowed by proxy)"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            break;
        case WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR:
            weechat_printf (NULL,
                            _("%s%s: unable to set local hostname/IP"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            break;
        case WEECHAT_HOOK_CONNECT_MEMORY_ERROR:
            weechat_printf (NULL,
                            _("%s%s: not enough memory (%s)"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            (error) ? error : "-");
            break;
        case WEECHAT_HOOK_CONNECT_TIMEOUT:
            weechat_printf (NULL,
                            _("%s%s: timeout"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            break;
        case WEECHAT_HOOK_CONNECT_SOCKET_ERROR:
            weechat_printf (NULL,
                            _("%s%s: unable to create socket"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME);
            break;
        default:
            weechat_printf (NULL,
                            _("%s%s: unable to connect: unexpected error (%d)"),
                            weechat_prefix ("error"), XFER_PLUGIN_NAME,
                            status);
            break;
    }
    if (error && error[0])
    {
        weechat_printf (NULL,
                        _("%s%s: error: %s"),
                        weechat_prefix ("error"), XFER_PLUGIN_NAME, error);
    }

    xfer_close (xfer, XFER_STATUS_FAILED);
    xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);

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

    if (XFER_IS_SEND(xfer->type))
    {
        /* create socket */
        if (xfer->sock < 0)
        {
            xfer->sock = socket (xfer->local_address->sa_family, SOCK_STREAM,
                                 0);
            if (xfer->sock < 0)
                return 0;
        }

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
                                         xfer, NULL);

        /* add timeout */
        if (weechat_config_integer (xfer_config_network_timeout) > 0)
        {
            xfer->hook_timer = weechat_hook_timer (weechat_config_integer (xfer_config_network_timeout) * 1000,
                                                   0, 1,
                                                   &xfer_network_timer_cb,
                                                   xfer, NULL);
        }
    }

    /* for chat receiving, connect to listening host */
    if (xfer->type == XFER_TYPE_CHAT_RECV)
    {
        xfer->hook_connect = weechat_hook_connect (xfer->proxy,
                                                   xfer->remote_address_str,
                                                   xfer->port, 1, 0, NULL, NULL,
                                                   0, "NONE", NULL,
                                                   &xfer_network_connect_chat_recv_cb,
                                                   xfer, NULL);
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
    }
    else
    {
        /* for a file: launch child process */
        if (XFER_IS_FILE(xfer->type))
            xfer_network_recv_file_fork (xfer);

        xfer->status = XFER_STATUS_CONNECTING;
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
