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
 * xfer-dcc.c: file transfer via DCC protocol
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-config.h"
#include "xfer-file.h"
#include "xfer-network.h"


/*
 * xfer_dcc_send_file_child: child process for sending file with DCC protocol
 */

void
xfer_dcc_send_file_child (struct t_xfer *xfer)
{
    int num_read, num_sent, blocksize;
    static char buffer[XFER_BLOCKSIZE_MAX];
    uint32_t ack;
    time_t last_sent, new_time, last_second, sent_ok;
    unsigned long long sent_last_second;

    blocksize = xfer->blocksize;
    if (weechat_config_integer (xfer_config_network_speed_limit) > 0)
    {
        if (blocksize > weechat_config_integer (xfer_config_network_speed_limit) * 1024)
            blocksize = weechat_config_integer (xfer_config_network_speed_limit) * 1024;
    }

    last_sent = time (NULL);
    last_second = time (NULL);
    sent_ok = 0;
    sent_last_second = 0;
    while (1)
    {
        /* read DCC ACK (sent by receiver) */
        if (xfer->pos > xfer->ack)
        {
            /* we should receive ACK for packets sent previously */
            while (1)
            {
                num_read = recv (xfer->sock, (char *) &ack, 4, MSG_PEEK);
                if ((num_read < 1) &&
                    ((num_read != -1) || ((errno != EAGAIN) && (errno != EWOULDBLOCK))))
                {
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_SEND_BLOCK);
                    return;
                }
                if (num_read == 4)
                {
                    recv (xfer->sock, (char *) &ack, 4, 0);
                    xfer->ack = ntohl (ack);

                    /* DCC send ok? */
                    if ((xfer->pos >= xfer->size)
                        && (xfer->ack >= xfer->size))
                    {
                        xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                                 XFER_NO_ERROR);
                        return;
                    }
                }
                else
                    break;
            }
        }

        /* send a block to receiver */
        if ((xfer->pos < xfer->size) &&
             (xfer->fast_send || (xfer->pos <= xfer->ack)))
        {
            if ((weechat_config_integer (xfer_config_network_speed_limit) > 0)
                && (sent_last_second >= (unsigned long long)weechat_config_integer (xfer_config_network_speed_limit) * 1024))
            {
                /* we're sending too fast (according to speed limit set by user) */
                usleep (100);
            }
            else
            {
                lseek (xfer->file, xfer->pos, SEEK_SET);
                num_read = read (xfer->file, buffer, blocksize);
                if (num_read < 1)
                {
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_READ_LOCAL);
                    return;
                }
                num_sent = send (xfer->sock, buffer, num_read, 0);
                if (num_sent < 0)
                {
                    /*
                     * socket is temporarily not available (receiver can't
                     * receive amount of data we sent ?!)
                     */
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        usleep (1000);
                    else
                    {
                        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                                 XFER_ERROR_SEND_BLOCK);
                        return;
                    }
                }
                if (num_sent > 0)
                {
                    xfer->pos += (unsigned long long) num_sent;
                    sent_last_second += (unsigned long long) num_sent;
                    new_time = time (NULL);
                    if ((last_sent != new_time)
                        || ((sent_ok == 0) && (xfer->pos >= xfer->size)))
                    {
                        last_sent = new_time;
                        xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                                                 XFER_NO_ERROR);
                        if (xfer->pos >= xfer->size)
                            sent_ok = new_time;
                    }
                }
            }
        }
        else
            usleep (1000);

        new_time = time (NULL);
        if (new_time > last_second)
        {
            last_second = new_time;
            sent_last_second = 0;
        }

        /*
         * if send if ok since 2 seconds or more, and that no ack was received,
         * then consider it's ok
         */
        if ((sent_ok != 0) && (new_time > sent_ok + 2))
        {
            xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                     XFER_NO_ERROR);
            return;
        }
    }
}

/*
 * xfer_dcc_recv_file_child: child process for receiving file
 */

void
xfer_dcc_recv_file_child (struct t_xfer *xfer)
{
    int num_read;
    static char buffer[XFER_BLOCKSIZE_MAX];
    uint32_t pos;
    time_t last_sent, new_time;
    unsigned long long bytes_remaining;

    /* first connect to sender (blocking) */
    if (!weechat_network_connect_to (xfer->proxy, xfer->sock,
                                     xfer->remote_address, xfer->port))
    {
        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                 XFER_ERROR_CONNECT_SENDER);
        return;
    }

    /* connection is ok, change DCC status (inform parent process) */
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                             XFER_NO_ERROR);

    last_sent = time (NULL);
    while (1)
    {
        bytes_remaining = xfer->size - xfer->pos;
        num_read = recv (xfer->sock, buffer,
                         (bytes_remaining >= sizeof (buffer)) ? sizeof (buffer) : bytes_remaining,
                         0);
        if (num_read == -1)
        {
            /* socket is temporarily not available (sender is not fast ?!) */
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
                usleep (1000);
            else
            {
                xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                         XFER_ERROR_RECV_BLOCK);
                return;
            }
        }
        else
        {
            if (num_read == 0)
            {
                xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                         XFER_ERROR_RECV_BLOCK);
                return;
            }

            if (write (xfer->file, buffer, num_read) == -1)
            {
                xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                         XFER_ERROR_WRITE_LOCAL);
                return;
            }

            xfer->pos += (unsigned long long) num_read;
            pos = htonl (xfer->pos);

            /* we don't check return code, not a problem if an ACK send failed */
            send (xfer->sock, (char *) &pos, 4, 0);

            /* file received ok? */
            if (xfer->pos >= xfer->size)
            {
                xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                         XFER_NO_ERROR);
                return;
            }

            new_time = time (NULL);
            if (last_sent != new_time)
            {
                last_sent = new_time;
                xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                                         XFER_NO_ERROR);
            }
        }
    }
}
