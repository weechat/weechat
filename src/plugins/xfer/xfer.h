/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_XFER_H
#define WEECHAT_PLUGIN_XFER_H

#include <unistd.h>
#include <time.h>
#include <gcrypt.h>
#include <sys/socket.h>

#define weechat_plugin weechat_xfer_plugin
#define XFER_PLUGIN_NAME "xfer"
#define XFER_PLUGIN_PRIORITY 7000

/* xfer types */

enum t_xfer_type
{
    XFER_TYPE_FILE_RECV_ACTIVE = 0,
    XFER_TYPE_FILE_RECV_PASSIVE,
    XFER_TYPE_FILE_SEND_ACTIVE,
    XFER_TYPE_FILE_SEND_PASSIVE,
    XFER_TYPE_CHAT_RECV,
    XFER_TYPE_CHAT_SEND,
    /* number of xfer types */
    XFER_NUM_TYPES,
};

/* xfer protocol (for file transfer) */

enum t_xfer_protocol
{
    XFER_NO_PROTOCOL = 0,
    XFER_PROTOCOL_DCC,
    /* number of xfer protocols */
    XFER_NUM_PROTOCOLS,
};

/* xfer status */

enum t_xfer_status
{
    XFER_STATUS_WAITING = 0,           /* waiting for host answer           */
    XFER_STATUS_CONNECTING,            /* connecting to host                */
    XFER_STATUS_ACTIVE,                /* sending/receiving data            */
    XFER_STATUS_DONE,                  /* transfer done                     */
    XFER_STATUS_FAILED,                /* transfer failed                   */
    XFER_STATUS_ABORTED,               /* transfer aborted by user          */
    XFER_STATUS_HASHING,               /* partial local file is being hashed*/
    XFER_STATUS_HASHED,                /* received file has been hashed     */
    /* number of xfer status */
    XFER_NUM_STATUS,
};

/* xfer errors */

enum t_xfer_error
{
    XFER_NO_ERROR = 0,                 /* no error to report, all OK!       */
    /* errors for sender: */
    XFER_ERROR_READ_LOCAL,             /* unable to read local file         */
    XFER_ERROR_SEND_BLOCK,             /* unable to send block to receiver  */
    XFER_ERROR_READ_ACK,               /* unable to read ACK from receiver  */
    /* errors for receiver: */
    XFER_ERROR_CONNECT_SENDER,         /* unable to connect to sender       */
    XFER_ERROR_RECV_BLOCK,             /* unable to recv block from sender  */
    XFER_ERROR_WRITE_LOCAL,            /* unable to write to local file     */
    XFER_ERROR_SEND_ACK,               /* unable to send ACK to sender      */
    XFER_ERROR_HASH_MISMATCH,          /* CRC32 does not match              */
    XFER_ERROR_HASH_RESUME_ERROR,      /* other error with CRC32 hash       */
    /* number of errors */
    XFER_NUM_ERRORS,
};

/* hash status */

enum t_xfer_hash_status
{
    XFER_HASH_STATUS_UNKNOWN = 0,      /* hashing not being performed       */
    XFER_HASH_STATUS_IN_PROGRESS,      /* hash in progress                  */
    XFER_HASH_STATUS_MATCH,            /* hash finished and matches target  */
    XFER_HASH_STATUS_MISMATCH,         /* hash finished with mismatch       */
    XFER_HASH_STATUS_RESUME_ERROR,     /* hash failed while resuming        */
    /* number of hash status */
    XFER_NUM_HASH_STATUS,
};

/* xfer block size */

#define XFER_BLOCKSIZE_MIN    1024     /* min block size                    */
#define XFER_BLOCKSIZE_MAX  102400     /* max block size                    */

/* separator in filenames */

#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif /* _WIN32 */

/* macros for type/status */

#define XFER_IS_FILE(type) ((type == XFER_TYPE_FILE_RECV_ACTIVE) ||  \
                            (type == XFER_TYPE_FILE_RECV_PASSIVE) || \
                            (type == XFER_TYPE_FILE_SEND_ACTIVE) ||  \
                            (type == XFER_TYPE_FILE_SEND_PASSIVE))
#define XFER_IS_CHAT(type) ((type == XFER_TYPE_CHAT_RECV) ||    \
                            (type == XFER_TYPE_CHAT_SEND))
#define XFER_IS_RECV(type) ((type == XFER_TYPE_FILE_RECV_ACTIVE) ||  \
                            (type == XFER_TYPE_FILE_RECV_PASSIVE) || \
                            (type == XFER_TYPE_CHAT_RECV))
#define XFER_IS_SEND(type) ((type == XFER_TYPE_FILE_SEND_ACTIVE) ||  \
                            (type == XFER_TYPE_FILE_SEND_PASSIVE) || \
                            (type == XFER_TYPE_CHAT_SEND))
#define XFER_IS_ACTIVE(type) ((type == XFER_TYPE_FILE_RECV_ACTIVE) || \
                              (type == XFER_TYPE_FILE_SEND_ACTIVE))
#define XFER_IS_FILE_PASSIVE(type) ((type == XFER_TYPE_FILE_RECV_PASSIVE) || \
                                    (type == XFER_TYPE_FILE_SEND_PASSIVE))

#define XFER_HAS_ENDED(status) ((status == XFER_STATUS_DONE) ||      \
                                (status == XFER_STATUS_FAILED) ||    \
                                (status == XFER_STATUS_ABORTED))

struct t_xfer
{
    /* data received by xfer to initiate a transfer */
    char *plugin_name;                 /* plugin name                       */
    char *plugin_id;                   /* id used by plugin                 */
    enum t_xfer_type type;             /* xfer type (send/recv file)        */
    enum t_xfer_protocol protocol;     /* xfer protocol (for file transfer) */
    char *remote_nick;                 /* remote nick                       */
    char *local_nick;                  /* local nick                        */
    char *charset_modifier;            /* string for charset modifier_data  */
    char *filename;                    /* filename                          */
    unsigned long long size;           /* file size                         */
    char *proxy;                       /* proxy to use (optional)           */
    struct sockaddr *local_address;    /* local IP address                  */
    socklen_t local_address_length;    /* local sockaddr length             */
    char *local_address_str;           /* local IP address as string        */
    struct sockaddr *remote_address;   /* remote IP address                 */
    socklen_t remote_address_length;   /* remote sockaddr length            */
    char *remote_address_str;          /* remote IP address as string       */
    int port;                          /* remote port                       */
    char *token;                       /* remote passive-DCC token          */

    /* internal data */
    enum t_xfer_status status;         /* xfer status (waiting, sending,..) */
    struct t_gui_buffer *buffer;       /* buffer (for chat only)            */
    char *remote_nick_color;           /* color name for remote nick        */
                                       /* (returned by IRC plugin)          */
    int fast_send;                     /* fast send file: does not wait ACK */
    int send_ack;                      /* send acks on file receive         */
    int blocksize;                     /* block size for sending file       */
    time_t start_time;                 /* time when xfer started            */
    struct timeval start_transfer;     /* time when xfer transfer started   */
    int sock;                          /* socket for connection             */
    pid_t child_pid;                   /* pid of child process (send/recv)  */
    int child_read;                    /* to read into child pipe           */
    int child_write;                   /* to write into child pipe          */
    struct t_hook *hook_fd;            /* hook for socket or child pipe     */
    struct t_hook *hook_timer;         /* timeout for receiver accept       */
    struct t_hook *hook_connect;       /* hook for connection to chat recv  */
    char *unterminated_message;        /* beginning of a message            */
    int file;                          /* local file (read or write)        */
    char *local_filename;              /* local filename (with path)        */
    char *temp_local_filename;         /* local filename with temp. suffix  */
                                       /* (during transfer, for receive     */
                                       /* file only)                        */
    int filename_suffix;               /* suffix (like .1) if renaming file */
    unsigned long long pos;            /* number of bytes received/sent     */
    unsigned long long ack;            /* number of bytes received OK       */
    unsigned long long start_resume;   /* start of resume (in bytes)        */
    struct timeval last_check_time;    /* last time we checked bytes snt/rcv*/
    unsigned long long last_check_pos; /* bytes sent/recv at last check     */
    time_t last_activity;              /* time of last byte received/sent   */
    unsigned long long bytes_per_sec;  /* bytes per second                  */
    unsigned long long eta;            /* estimated time of arrival         */
    gcry_md_hd_t *hash_handle;         /* handle for CRC32 hash             */
    char *hash_target;                 /* the CRC32 hash to check against   */
    enum t_xfer_hash_status hash_status; /* hash status                     */
    struct t_xfer *prev_xfer;          /* link to previous xfer             */
    struct t_xfer *next_xfer;          /* link to next xfer                 */
};

extern struct t_weechat_plugin *weechat_xfer_plugin;
extern char *xfer_type_string[];
extern char *xfer_protocol_string[];
extern char *xfer_status_string[];
extern char *xfer_hash_status_string[];
extern struct t_xfer *xfer_list, *last_xfer;
extern int xfer_count;

extern int xfer_valid (struct t_xfer *xfer);
extern struct t_xfer *xfer_search_by_number (int number);
extern struct t_xfer *xfer_search_by_buffer (struct t_gui_buffer *buffer);
extern void xfer_close (struct t_xfer *xfer, enum t_xfer_status status);
extern void xfer_send_signal (struct t_xfer *xfer, const char *signal);
extern void xfer_set_remote_address (struct t_xfer *xfer,
                                     const struct sockaddr *address,
                                     socklen_t length, const char *address_str);
extern void xfer_set_local_address (struct t_xfer *xfer,
                                    const struct sockaddr *address,
                                    socklen_t length, const char *address_str);
extern void xfer_free (struct t_xfer *xfer);
extern int xfer_add_to_infolist (struct t_infolist *infolist,
                                 struct t_xfer *xfer);

#endif /* WEECHAT_PLUGIN_XFER_H */
