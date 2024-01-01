/*
 * xfer-dcc.c - file transfer via DCC protocol
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <gcrypt.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-config.h"
#include "xfer-file.h"
#include "xfer-network.h"


/*
 * Child process for sending file with DCC protocol.
 */

void
xfer_dcc_send_file_child (struct t_xfer *xfer)
{
    int num_read, num_sent;
    static char buffer[XFER_BLOCKSIZE_MAX];
    uint32_t ack;
    time_t last_sent, new_time, last_second, sent_ok;
    unsigned long long blocksize, speed_limit, sent_last_second;

    /* empty file? just return immediately */
    if (xfer->pos >= xfer->size)
    {
        xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                 XFER_NO_ERROR);
        return;
    }

    speed_limit = (unsigned long long)weechat_config_integer (
        xfer_config_network_speed_limit_send);

    blocksize = (unsigned long long)(xfer->blocksize);
    if ((speed_limit > 0) && (blocksize > speed_limit * 1024))
        blocksize = speed_limit * 1024;

    last_sent = time (NULL);
    last_second = last_sent;
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

                    /* DCC send OK? */
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
            if ((speed_limit > 0) && (sent_last_second >= speed_limit * 1024))
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
         * if send if OK since 2 seconds or more, and that no ACK was received,
         * then consider it's OK
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
 * Sends ACK to sender using current position in file received.
 *
 * Returns:
 *   2: ACK sent successfully (the 4 bytes)
 *   1: ACK not sent, but we consider it's not a problem
 *   0: ACK not sent with socket error (DCC will be closed)
 */

int
xfer_dcc_recv_file_send_ack (struct t_xfer *xfer)
{
    int length, num_sent, total_sent;
    uint32_t pos;
    const void *ptr_buf;

    pos = htonl (xfer->pos);
    ptr_buf = &pos;
    length = 4;
    total_sent = 0;
    num_sent = send (xfer->sock, ptr_buf, length, 0);
    if (num_sent > 0)
        total_sent += num_sent;
    while (total_sent < length)
    {
        if ((num_sent == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
            return 0;

        /* if we can't send ACK now, just return with partial failure code */
        if (total_sent == 0)
            return 1;

        /* at least one byte has been sent, we must send whole ACK */
        usleep (1000);
        num_sent = send (xfer->sock, ptr_buf + total_sent,
                         length - total_sent, 0);
        if (num_sent > 0)
            total_sent += num_sent;
    }

    /* ACK successfully sent */
    return 2;
}

/*
 * Reads a resumed xfer from disk for hashing.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
xfer_dcc_resume_hash (struct t_xfer *xfer)
{
    char *buf;
    unsigned long long total_read;
    ssize_t length_buf, to_read, num_read;
    int ret, fd;

    total_read = 0;
    ret = 1;
    fd = 0;

    length_buf = 1024 * 1024;
    buf = malloc (length_buf);
    if (!buf)
        return 0;

    while (fd <= 0)
    {
        fd = open (xfer->temp_local_filename, O_RDONLY);
        if (fd < 0)
        {
            if (errno == EINTR)
                continue;
            fd = 0;
            ret = 0;
            break;
        }
    }

    if (fd)
    {
        while (total_read < xfer->start_resume)
        {
            to_read = xfer->start_resume - total_read;
            if (to_read > length_buf)
                num_read = read (fd, buf, length_buf);
            else
                num_read = read (fd, buf, to_read);
            if (num_read > 0)
            {
                gcry_md_write (*xfer->hash_handle, buf, num_read);
                total_read += num_read;
            }
            else if (num_read < 0)
            {
                if (errno == EINTR)
                    continue;
                ret = 0;
                break;
            }
        }

        while (close (fd) < 0)
        {
            if (errno != EINTR)
                break;
        }
    }

    free (buf);

    return ret;
}

/*
 * Child process for receiving file with DCC protocol.
 */

void
xfer_dcc_recv_file_child (struct t_xfer *xfer)
{
    int flags, num_read, ready;
    static char buffer[XFER_BLOCKSIZE_MAX];
    time_t last_sent, last_second, new_time;
    unsigned long long blocksize, pos_last_ack, speed_limit, recv_last_second;
    struct pollfd poll_fd;
    ssize_t written, total_written;
    unsigned char *bin_hash;
    char hash[9];

    speed_limit = (unsigned long long)weechat_config_integer (
        xfer_config_network_speed_limit_recv);

    blocksize = sizeof (buffer);
    if ((speed_limit > 0) && (blocksize > speed_limit * 1024))
        blocksize = speed_limit * 1024;

    /* if resuming, hash the portion of the file we have */
    if ((xfer->start_resume > 0) && xfer->hash_handle)
    {
        xfer_network_write_pipe (xfer, XFER_STATUS_HASHING,
                                 XFER_NO_ERROR);
        if (!xfer_dcc_resume_hash (xfer))
        {
            gcry_md_close (*xfer->hash_handle);
            free (xfer->hash_handle);
            xfer->hash_handle = NULL;
            xfer_network_write_pipe (xfer, XFER_STATUS_HASHING,
                                     XFER_ERROR_HASH_RESUME_ERROR);
        }
        xfer_network_write_pipe (xfer, XFER_STATUS_CONNECTING,
                                 XFER_NO_ERROR);
    }

    /* first connect to sender (blocking) */
    if (xfer->type == XFER_TYPE_FILE_RECV_ACTIVE)
    {
        xfer->sock = weechat_network_connect_to (xfer->proxy,
                                                 xfer->remote_address,
                                                 xfer->remote_address_length);
        if (xfer->sock == -1)
        {
            xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                     XFER_ERROR_CONNECT_SENDER);
            return;
        }
    }

    /* set TCP_NODELAY to be more aggressive with acks */
    flags = 1;
    setsockopt (xfer->sock, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof (flags));

    /* connection is OK, change DCC status (inform parent process) */
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                             XFER_NO_ERROR);

    /* make socket non-blocking */
    flags = fcntl (xfer->sock, F_GETFL);
    if (flags == -1)
        flags = 0;
    fcntl (xfer->sock, F_SETFL, flags | O_NONBLOCK);

    last_sent = time (NULL);
    last_second = last_sent;
    recv_last_second = 0;
    pos_last_ack = 0;

    while (1)
    {
        /* wait until there is something to read on socket (or error) */
        poll_fd.fd = xfer->sock;
        poll_fd.events = POLLIN;
        poll_fd.revents = 0;
        ready = poll (&poll_fd, 1, -1);
        if (ready <= 0)
        {
            if ((errno == EINTR) || (errno == EAGAIN))
                continue;
            else
            {
                xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                         XFER_ERROR_RECV_BLOCK);
                return;
            }
        }

        /* read maximum data on socket (until nothing is available) */
        while (1)
        {
            if ((speed_limit > 0) && (recv_last_second >= speed_limit * 1024))
            {
                /*
                 * we're receiving too fast
                 * (according to speed limit set by user)
                 */
                usleep (100);
            }
            else
            {
                num_read = recv (xfer->sock, buffer, blocksize, 0);
                if (num_read == -1)
                {
                    if ((errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR))
                    {
                        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                                 XFER_ERROR_RECV_BLOCK);
                        return;
                    }
                    /*
                     * no more data available on socket: exit loop, send ACK, and
                     * wait for new data on socket
                     */
                    break;
                }
                else
                {
                    if ((num_read == 0) && (xfer->pos < xfer->size))
                    {
                        xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                                 XFER_ERROR_RECV_BLOCK);
                        return;
                    }

                    /* bytes received, write to disk */
                    total_written = 0;
                    while (total_written < num_read)
                    {
                        written = write (xfer->file,
                                         buffer + total_written,
                                         num_read - total_written);
                        if (written < 0)
                        {
                            if (errno == EINTR)
                                continue;
                            xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                                     XFER_ERROR_WRITE_LOCAL);
                            return;
                        }
                        else
                        {
                            if (xfer->hash_handle)
                            {
                                gcry_md_write (*xfer->hash_handle,
                                               buffer + total_written,
                                               written);
                            }
                            total_written += written;
                        }
                    }

                    xfer->pos += (unsigned long long) num_read;
                    recv_last_second += (unsigned long long) num_read;

                    /* file received OK? */
                    if (xfer->pos >= xfer->size)
                    {
                        /* check hash and report result to pipe */
                        if (xfer->hash_handle)
                        {
                            gcry_md_final (*xfer->hash_handle);
                            bin_hash = gcry_md_read (*xfer->hash_handle, 0);
                            if (bin_hash)
                            {
                                snprintf (hash, sizeof (hash), "%.2X%.2X%.2X%.2X",
                                          bin_hash[0], bin_hash[1], bin_hash[2],
                                          bin_hash[3]);
                                if (weechat_strcasecmp (hash,
                                                        xfer->hash_target) == 0)
                                {
                                    xfer_network_write_pipe (xfer,
                                                             XFER_STATUS_HASHED,
                                                             XFER_NO_ERROR);
                                }
                                else
                                {
                                    xfer_network_write_pipe (xfer,
                                                             XFER_STATUS_HASHED,
                                                             XFER_ERROR_HASH_MISMATCH);
                                }
                            }
                        }

                        fsync (xfer->file);

                        /*
                         * extra delay before sending ACK, otherwise the send of ACK
                         * may fail
                         */
                        usleep (100000);

                        /* send ACK to sender without checking return code (file OK) */
                        xfer_dcc_recv_file_send_ack (xfer);

                        /* set status done and return */
                        xfer_network_write_pipe (xfer, XFER_STATUS_DONE,
                                                 XFER_NO_ERROR);
                        return;
                    }

                    /* update status of DCC (parent process) */
                    new_time = time (NULL);
                    if (last_sent != new_time)
                    {
                        last_sent = new_time;
                        xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                                                 XFER_NO_ERROR);
                    }
                }
            }

            new_time = time (NULL);
            if (new_time > last_second)
            {
                last_second = new_time;
                recv_last_second = 0;
            }
        }

        /* send ACK to sender (if needed) */
        if (xfer->send_ack && (xfer->pos > pos_last_ack))
        {
            switch (xfer_dcc_recv_file_send_ack (xfer))
            {
                case 0:
                    /* send error, socket down? */
                    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                                             XFER_ERROR_SEND_ACK);
                    return;
                case 1:
                    /* send error, not fatal (buffer full?): disable ACKs */
                    xfer->send_ack = 0;
                    break;
                case 2:
                    /* send OK: save position in file as last ACK sent */
                    pos_last_ack = xfer->pos;
                    break;
            }
        }
    }
}
