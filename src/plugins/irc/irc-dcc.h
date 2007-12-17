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


#ifndef __WEECHAT_IRC_DCC_H
#define __WEECHAT_IRC_DCC_H 1

#include "irc-server.h"
#include "irc-channel.h"

/* DCC types */

#define IRC_DCC_CHAT_RECV        0  /* receiving DCC chat                    */
#define IRC_DCC_CHAT_SEND        1  /* sending DCC chat                      */
#define IRC_DCC_FILE_RECV        2  /* incoming DCC file                     */
#define IRC_DCC_FILE_SEND        3  /* sending DCC file                      */

/* DCC status */

#define IRC_DCC_WAITING          0  /* waiting for host answer               */
#define IRC_DCC_CONNECTING       1  /* connecting to host                    */
#define IRC_DCC_ACTIVE           2  /* sending/receiving data                */
#define IRC_DCC_DONE             3  /* transfer done                         */
#define IRC_DCC_FAILED           4  /* DCC failed                            */
#define IRC_DCC_ABORTED          5  /* DCC aborted by user                   */

/* DCC blocksize (for file) */

#define IRC_DCC_MIN_BLOCKSIZE   1024  /* min DCC block size when sending file*/
#define IRC_DCC_MAX_BLOCKSIZE 102400  /* max DCC block size when sending file*/

/* DCC errors (for file) */

#define IRC_DCC_NO_ERROR             0  /* no error to report, all ok!       */
#define IRC_DCC_ERROR_READ_LOCAL     1  /* unable to read local file         */
#define IRC_DCC_ERROR_SEND_BLOCK     2  /* unable to send block to receiver  */
#define IRC_DCC_ERROR_READ_ACK       3  /* unable to read ACK from receiver  */
#define IRC_DCC_ERROR_CONNECT_SENDER 4  /* unable to connect to sender       */
#define IRC_DCC_ERROR_RECV_BLOCK     5  /* unable to recv block from sender  */
#define IRC_DCC_ERROR_WRITE_LOCAL    6  /* unable to write to local file     */

/* DCC macros for type */

#define IRC_DCC_IS_CHAT(type) ((type == IRC_DCC_CHAT_RECV) || \
                               (type == IRC_DCC_CHAT_SEND))
#define IRC_DCC_IS_FILE(type) ((type == IRC_DCC_FILE_RECV) || \
                               (type == IRC_DCC_FILE_SEND))
#define IRC_DCC_IS_RECV(type) ((type == IRC_DCC_CHAT_RECV) || \
                               (type == IRC_DCC_FILE_RECV))
#define IRC_DCC_IS_SEND(type) ((type == IRC_DCC_CHAT_SEND) || \
                               (type == IRC_DCC_FILE_SEND))

/* DCC macro for status */

#define IRC_DCC_ENDED(status) ((status == IRC_DCC_DONE) || \
                               (status == IRC_DCC_FAILED) || \
                               (status == IRC_DCC_ABORTED))

struct t_irc_dcc
{
    struct t_irc_server *server;    /* irc server                            */
    struct t_irc_channel *channel;  /* irc channel (for DCC chat only)       */
    int type;                       /* DCC type (file/chat, send/receive)    */
    int status;                     /* DCC status (waiting, sending, ..)     */
    time_t start_time;              /* the time when DCC started             */
    time_t start_transfer;          /* the time when DCC transfer started    */
    unsigned long addr;             /* IP address                            */
    int port;                       /* port                                  */
    char *nick;                     /* remote nick                           */
    int sock;                       /* socket for connection                 */
    pid_t child_pid;                /* pid of child process (sending/recving)*/
    int child_read;                 /* to read into child pipe               */
    int child_write;                /* to write into child pipe              */
    char *unterminated_message;     /* beginning of a message in input buf   */
    int fast_send;                  /* fase send for files: does not wait ACK*/
    int file;                       /* local file (for reading or writing)   */
    char *filename;                 /* filename (given by sender)            */
    char *local_filename;           /* local filename (with path)            */
    int filename_suffix;            /* suffix (.1 for ex) if renaming file   */
    int blocksize;                  /* block size for sending file           */
    unsigned long size;             /* file size                             */
    unsigned long pos;              /* number of bytes received/sent         */
    unsigned long ack;              /* number of bytes received OK           */
    unsigned long start_resume;     /* start of resume (in bytes)            */
    time_t last_check_time;         /* last time we looked at bytes sent/recv*/
    unsigned long last_check_pos;   /* bytes sent/recv at last check         */
    time_t last_activity;           /* time of last byte received/sent       */
    unsigned long bytes_per_sec;    /* bytes per second                      */
    unsigned long eta;              /* estimated time of arrival             */
    struct t_irc_dcc *prev_dcc;     /* link to previous dcc file/chat        */
    struct t_irc_dcc *next_dcc;     /* link to next dcc file/chat            */
};

extern struct t_irc_dcc *irc_dcc_list;
extern struct t_irc_dcc *irc_last_dcc;
extern char *irc_dcc_status_string[6];

extern void irc_dcc_redraw (int highlight);
extern void irc_dcc_free (struct t_irc_dcc *dcc);
extern void irc_dcc_close (struct t_irc_dcc *dcc, int status);
extern void irc_dcc_chat_remove_channel (struct t_irc_channel *channel);
extern void irc_dcc_accept (struct t_irc_dcc *dcc);
extern void irc_dcc_accept_resume (struct t_irc_server *server, char *filename,
                                   int port, unsigned long pos_start);
extern void irc_dcc_start_resume (struct t_irc_server *server, char *filename,
                                  int port, unsigned long pos_start);
extern struct t_irc_dcc *irc_dcc_alloc ();
extern struct t_irc_dcc *irc_dcc_add (struct t_irc_server *server,
                                      int type, unsigned long addr,
                                      int port, char *nick, int sock,
                                      char *filename, char *local_filename,
                                      unsigned long size);
extern void irc_dcc_send_request (struct t_irc_server *server, int type,
                                  char *nick, char *filename);
extern void irc_dcc_chat_sendf (struct t_irc_dcc *dcc, char *format, ...);
extern void irc_dcc_file_send_fork (struct t_irc_dcc *dcc);
extern void irc_dcc_file_recv_fork (struct t_irc_dcc *dcc);
extern void irc_dcc_handle ();
extern void irc_dcc_end ();
extern void irc_dcc_print_log ();

#endif /* irc-dcc.h */
