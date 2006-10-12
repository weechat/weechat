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


#ifndef __WEECHAT_IRC_H
#define __WEECHAT_IRC_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <regex.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "../gui/gui.h"

/* prefixes for chat window */

#define PREFIX_SERVER    "-@-"
#define PREFIX_INFO      "-=-"
#define PREFIX_ACTION_ME "-*-"
#define PREFIX_JOIN      "-->"
#define PREFIX_PART      "<--"
#define PREFIX_QUIT      "<--"
#define PREFIX_ERROR     "=!="
#define PREFIX_PLUGIN    "-P-"

#define DEFAULT_IRC_PORT 6667

/* nick types */

#define NICK_CHANOWNER 1
#define NICK_CHANADMIN 2
#define NICK_OP        4
#define NICK_HALFOP    8
#define NICK_VOICE     16
#define NICK_AWAY      32
#define NICK_SET_FLAG(nick, set, flag) \
    if (set) \
        nick->flags |= flag; \
    else \
        nick->flags &= 0xFFFF - flag;

typedef struct t_irc_nick t_irc_nick;

struct t_irc_nick
{
    char *nick;                     /* nickname                               */
    char *host;                     /* full hostname                          */
    int flags;                      /* chanowner/chanadmin (unrealircd),      */
                                    /* op, halfop, voice, away                */
    int color;                      /* color for nickname in chat window      */
    t_irc_nick *prev_nick;          /* link to previous nick on the channel   */
    t_irc_nick *next_nick;          /* link to next nick on the channel       */
};

/* channel types */

#define CHANNEL_PREFIX "#&+!"

#define CHANNEL_TYPE_UNKNOWN  -1
#define CHANNEL_TYPE_CHANNEL  0
#define CHANNEL_TYPE_PRIVATE  1
#define CHANNEL_TYPE_DCC_CHAT 2

typedef struct t_irc_channel t_irc_channel;

struct t_irc_channel
{
    int type;                       /* channel type                           */
    void *dcc_chat;                 /* DCC CHAT pointer (NULL if not DCC)     */
    char *name;                     /* name of channel (exemple: "#abc")      */
    char *topic;                    /* topic of channel (host for private)    */
    char *modes;                    /* channel modes                          */
    int limit;                      /* user limit (0 is limit not set)        */
    char *key;                      /* channel key (NULL if no key is set)    */
    int nicks_count;                /* # nicks on channel (0 if dcc/pv)       */
    int checking_away;              /* = 1 if checking away with WHO cmd      */
    char *away_message;             /* to display away only once in private   */
    int cycle;                      /* currently cycling (/part then /join)   */
    int close;                      /* close request (/buffer close)          */
    int display_creation_date;      /* 1 if creation date should be displayed */
    t_irc_nick *nicks;              /* nicks on the channel                   */
    t_irc_nick *last_nick;          /* last nick on the channel               */
    t_gui_buffer *buffer;           /* GUI buffer allocated for channel       */
    t_irc_channel *prev_channel;    /* link to previous channel               */
    t_irc_channel *next_channel;    /* link to next channel                   */
};

/* server types */

typedef struct t_irc_server t_irc_server;

struct t_irc_server
{
    /* user choices */
    char *name;                     /* name of server (only for display)      */
    int autoconnect;                /* = 1 if auto connect at startup         */
    int autoreconnect;              /* = 1 if auto reco when disconnected     */
    int autoreconnect_delay;        /* delay before trying again reconnect    */
    int command_line;               /* server was given on command line       */
    char *address;                  /* address of server (IP or name)         */
    int port;                       /* port for server (6667 by default)      */
    int ipv6;                       /* use IPv6 protocol                      */
    int ssl;                        /* SSL protocol                           */
    char *password;                 /* password for server                    */
    char *nick1;                    /* first nickname for the server          */
    char *nick2;                    /* alternate nickname                     */
    char *nick3;                    /* 2nd alternate nickname                 */
    char *username;                 /* user name                              */
    char *realname;                 /* real name                              */
    char *hostname;                 /* custom hostname                        */
    char *command;                  /* command to run once connected          */
    int command_delay;              /* delay after execution of command       */
    char *autojoin;                 /* channels to automatically join         */
    int autorejoin;                 /* auto rejoin channels when kicked       */
    char *notify_levels;            /* channels notify levels                 */
    char *charset_decode_iso;       /* channels charsets for decoding ISO     */
    char *charset_decode_utf;       /* channels charsets for decoding UTF     */
    char *charset_encode;           /* channels charsets for encoding msgs    */
    
    /* internal vars */
    pid_t child_pid;                /* pid of child process (connecting)      */
    int child_read;                 /* to read into child pipe                */
    int child_write;                /* to write into child pipe               */
    int sock;                       /* socket for server (IPv4 or IPv6)       */
    int is_connected;               /* 1 if WeeChat is connected to server    */
    int ssl_connected;              /* = 1 if connected with SSL              */
#ifdef HAVE_GNUTLS
    gnutls_session gnutls_sess;     /* gnutls session (only if SSL is used)   */
#endif
    char *unterminated_message;     /* beginning of a message in input buf    */
    char *nick;                     /* current nickname                       */
    char *nick_modes;               /* nick modes                             */
    time_t reconnect_start;         /* this time + delay = reconnect time     */
    int reconnect_join;             /* 1 if channels opened to rejoin         */
    int is_away;                    /* 1 is user is marked as away            */
    char *away_message;             /* away message, NULL if not away         */
    time_t away_time;               /* time() when user marking as away       */
    int lag;                        /* lag (in milliseconds)                  */
    struct timeval lag_check_time;  /* last time lag was checked (ping sent)  */
    time_t lag_next_check;          /* time for next check                    */
    regex_t *cmd_list_regexp;       /* compiled Regular Expression for /list  */ 
    t_gui_buffer *buffer;           /* GUI buffer allocated for server        */
    t_gui_buffer *saved_buffer;     /* channel before jumping to next server  */
    t_irc_channel *channels;        /* opened channels on server              */
    t_irc_channel *last_channel;    /* last opened channal on server          */
    t_irc_server *prev_server;      /* link to previous server                */
    t_irc_server *next_server;      /* link to next server                    */
};

/* irc commands */

typedef int (t_irc_recv_func)(t_irc_server *, char *, char *, char *);

typedef struct t_irc_command t_irc_command;

struct t_irc_command
{
    char *command_name;             /* IRC command name                       */
    char *command_description;      /* command description (for /help)        */
    char *arguments;                /* command arguments (for /help)          */
    char *arguments_description;    /* arguments description (for /help)      */
    char *completion_template;      /* template for completion                */
                                    /* NULL=no completion, ""=default (nick)  */
    int min_arg, max_arg;           /* min & max number of arguments          */
    int conversion;                 /* = 1 if cmd args are converted (charset */
                                    /* and color) before sending to server    */
    int needs_connection;           /* = 1 if cmd needs server connection     */
    int (*cmd_function_args)(t_irc_server *, t_irc_channel *, int, char **);
                                    /* function called when user enters cmd   */
    int (*cmd_function_1arg)(t_irc_server *, t_irc_channel *, char *);
                                    /* function called when user enters cmd   */
    t_irc_recv_func *recv_function; /* function called when cmd is received   */
};

/* irc messages */

typedef struct t_irc_message t_irc_message;

struct t_irc_message
{
    t_irc_server *server;           /* server pointer for received msg        */
    char *data;                     /* message content                        */
    t_irc_message *next_message;    /* link to next message                   */
};

/* DCC types */

#define DCC_CHAT_RECV            0  /* receiving DCC chat                     */
#define DCC_CHAT_SEND            1  /* sending DCC chat                       */
#define DCC_FILE_RECV            2  /* incoming DCC file                      */
#define DCC_FILE_SEND            3  /* sending DCC file                       */

/* DCC status */

#define DCC_WAITING              0  /* waiting for host answer                */
#define DCC_CONNECTING           1  /* connecting to host                     */
#define DCC_ACTIVE               2  /* sending/receiving data                 */
#define DCC_DONE                 3  /* transfer done                          */
#define DCC_FAILED               4  /* DCC failed                             */
#define DCC_ABORTED              5  /* DCC aborted by user                    */

/* DCC blocksize (for file) */

#define DCC_MIN_BLOCKSIZE     1024  /* min DCC block size when sending file   */
#define DCC_MAX_BLOCKSIZE   102400  /* max DCC block size when sending file   */

/* DCC errors (for file) */

#define DCC_NO_ERROR             0  /* used when no error to report, all ok!  */
#define DCC_ERROR_READ_LOCAL     1  /* unable to read local file              */
#define DCC_ERROR_SEND_BLOCK     2  /* unable to send block to receiver       */
#define DCC_ERROR_READ_ACK       3  /* unable to read ACK from receiver       */
#define DCC_ERROR_CONNECT_SENDER 4  /* unable to connect to sender            */
#define DCC_ERROR_RECV_BLOCK     5  /* unable to receive block from sender    */
#define DCC_ERROR_WRITE_LOCAL    6  /* unable to write to local file          */

/* DCC macros for type */

#define DCC_IS_CHAT(type) ((type == DCC_CHAT_RECV) || (type == DCC_CHAT_SEND))
#define DCC_IS_FILE(type) ((type == DCC_FILE_RECV) || (type == DCC_FILE_SEND))
#define DCC_IS_RECV(type) ((type == DCC_CHAT_RECV) || (type == DCC_FILE_RECV))
#define DCC_IS_SEND(type) ((type == DCC_CHAT_SEND) || (type == DCC_FILE_SEND))

/* DCC macro for status */

#define DCC_ENDED(status) ((status == DCC_DONE) || (status == DCC_FAILED) || \
                          (status == DCC_ABORTED))

typedef struct t_irc_dcc t_irc_dcc;

struct t_irc_dcc
{
    t_irc_server *server;           /* irc server                             */
    t_irc_channel *channel;         /* irc channel (for DCC chat only)        */
    int type;                       /* DCC type (file/chat, send/receive)     */
    int status;                     /* DCC status (waiting, sending, ..)      */
    time_t start_time;              /* the time when DCC started              */
    time_t start_transfer;          /* the time when DCC transfer started     */
    unsigned long addr;             /* IP address                             */
    int port;                       /* port                                   */
    char *nick;                     /* remote nick                            */
    int sock;                       /* socket for connection                  */
    pid_t child_pid;                /* pid of child process (sending/recving) */
    int child_read;                 /* to read into child pipe                */
    int child_write;                /* to write into child pipe               */
    char *unterminated_message;     /* beginning of a message in input buf    */
    int fast_send;                  /* fase send for files: does not wait ACK */
    int file;                       /* local file (for reading or writing)    */
    char *filename;                 /* filename (given by sender)             */
    char *local_filename;           /* local filename (with path)             */
    int filename_suffix;            /* suffix (.1 for ex) if renaming file    */
    int blocksize;                  /* block size for sending file            */
    unsigned long size;             /* file size                              */
    unsigned long pos;              /* number of bytes received/sent          */
    unsigned long ack;              /* number of bytes received OK            */
    unsigned long start_resume;     /* start of resume (in bytes)             */
    time_t last_check_time;         /* last time we looked at bytes sent/recv */
    unsigned long last_check_pos;   /* bytes sent/recv at last check          */
    time_t last_activity;           /* time of last byte received/sent        */
    unsigned long bytes_per_sec;    /* bytes per second                       */
    unsigned long eta;              /* estimated time of arrival              */
    t_irc_dcc *prev_dcc;            /* link to previous dcc file/chat         */
    t_irc_dcc *next_dcc;            /* link to next dcc file/chat             */
};

/* ignore types */

/* pre-defined ignore types, all other types are made with IRC commands */
/* for example:  part  join  quit  notice  invite  ...                  */

#define IGNORE_ACTION  "action"
#define IGNORE_CTCP    "ctcp"
#define IGNORE_DCC     "dcc"
#define IGNORE_PRIVATE "pv"

typedef struct t_irc_ignore t_irc_ignore;

struct t_irc_ignore
{
    char *mask;                     /* nickname or mask                       */
    char *type;                     /* type of ignore                         */
    char *channel_name;             /* name of channel, "*" == all            */
    char *server_name;              /* name of server, "*" == all             */
    t_irc_ignore *prev_ignore;      /* pointer to previous ignore             */
    t_irc_ignore *next_ignore;      /* pointer to next ignore                 */
};

/* variables */

extern t_irc_command irc_commands[];
extern t_irc_server *irc_servers;
extern t_irc_message *recv_msgq, *msgq_last_msg;
extern int check_away;
extern t_irc_dcc *dcc_list;
extern t_irc_dcc *last_dcc;
extern char *dcc_status_string[6];
extern char *channel_modes;
extern char *nick_modes;
extern char *ignore_types[];
extern t_irc_ignore *irc_ignore;
extern t_irc_ignore *irc_last_ignore;

/* server functions (irc-server.c) */

extern void server_init (t_irc_server *);
extern int server_init_with_url (char *, t_irc_server *);
extern t_irc_server *server_alloc ();
extern void server_destroy (t_irc_server *);
extern void server_free (t_irc_server *);
extern void server_free_all ();
extern t_irc_server *server_new (char *, int, int, int, int, char *, int, int, int,
                                 char *, char *, char *, char *, char *, char *,
                                 char *, char *, int, char *, int, char *, char *,
                                 char *, char *);
extern char *server_get_charset_decode_iso (t_irc_server *);
extern char *server_get_charset_decode_utf (t_irc_server *);
extern char *server_get_charset_encode (t_irc_server *);
extern int server_send (t_irc_server *, char *, int);
extern void server_sendf (t_irc_server *, char *, ...);
extern void server_recv (t_irc_server *);
extern void server_child_read (t_irc_server *);
extern int server_connect (t_irc_server *);
extern void server_reconnect (t_irc_server *);
extern void server_auto_connect (int, int);
extern void server_disconnect (t_irc_server *, int);
extern void server_disconnect_all ();
extern t_irc_server *server_search (char *);
extern int server_get_number_connected ();
extern void server_get_number_buffer (t_irc_server *, int *, int *);
extern int server_name_already_exists (char *);
extern void server_remove_away ();
extern void server_check_away ();
extern void server_set_away (t_irc_server *, char *, int);
extern int server_get_default_notify_level (t_irc_server *);
extern void server_set_default_notify_level (t_irc_server *, int);
extern void server_print_log (t_irc_server *);

/* proxy functions (irc-server.c) */
extern void convbase64_8x3_to_6x4(char *from, char* to);
extern void base64encode(char *from, char *to);
extern int pass_httpproxy(int, char*, int);
extern int resolve(char *hostname, char *ip, int *version);
extern int pass_socks4proxy(int, char*, int, char*);
extern int pass_socks5proxy(int, char*, int);
extern int pass_proxy(int, char*, int, char*);

/* channel functions (irc-channel.c) */

extern t_irc_channel *channel_new (t_irc_server *, int, char *);
extern void channel_free (t_irc_server *, t_irc_channel *);
extern void channel_free_all (t_irc_server *);
extern t_irc_channel *channel_search (t_irc_server *, char *);
extern t_irc_channel *channel_search_any (t_irc_server *, char *);
extern t_irc_channel *channel_search_any_without_buffer (t_irc_server *, char *);
extern t_irc_channel *channel_search_dcc (t_irc_server *, char *);
extern int string_is_channel (char *);
extern char *channel_get_charset_decode_iso (t_irc_server *, t_irc_channel *);
extern char *channel_get_charset_decode_utf (t_irc_server *, t_irc_channel *);
extern char *channel_get_charset_encode (t_irc_server *, t_irc_channel *);
extern char *channel_iconv_decode (t_irc_server *, t_irc_channel *, char *);
extern char *channel_iconv_encode (t_irc_server *, t_irc_channel *, char *);
extern void channel_remove_away (t_irc_channel *);
extern void channel_check_away (t_irc_server *, t_irc_channel *, int);
extern void channel_set_away (t_irc_channel *, char *, int);
extern int channel_create_dcc (t_irc_dcc *);
extern int channel_get_notify_level (t_irc_server *, t_irc_channel *);
extern void channel_set_notify_level (t_irc_server *, t_irc_channel *, int);
extern void channel_print_log (t_irc_channel *);

/* nick functions (irc-nick.c) */

extern int nick_find_color (t_irc_nick *);
extern t_irc_nick *nick_new (t_irc_server *, t_irc_channel *, char *,
                             int, int, int, int, int);
extern void nick_resort (t_irc_channel *, t_irc_nick *);
extern void nick_change (t_irc_channel *, t_irc_nick *, char *);
extern void nick_free (t_irc_channel *, t_irc_nick *);
extern void nick_free_all (t_irc_channel *);
extern t_irc_nick *nick_search (t_irc_channel *, char *);
extern void nick_count (t_irc_channel *, int *, int *, int *, int *, int *);
extern int nick_get_max_length (t_irc_channel *);
extern void nick_set_away (t_irc_channel *, t_irc_nick *, int);
extern void nick_print_log (t_irc_nick *);

/* mode functions (irc-mode.c) */

extern void irc_mode_channel_set (t_irc_channel *, char *);
extern void irc_mode_user_set (t_irc_server *, char *);

/* DCC functions (irc-dcc.c) */

extern void dcc_redraw (int);
extern void dcc_free (t_irc_dcc *);
extern void dcc_close (t_irc_dcc *, int);
extern void dcc_chat_remove_channel (t_irc_channel *);
extern void dcc_accept (t_irc_dcc *);
extern void dcc_accept_resume (t_irc_server *, char *, int, unsigned long);
extern void dcc_start_resume (t_irc_server *, char *, int, unsigned long);
extern t_irc_dcc *dcc_alloc ();
extern t_irc_dcc *dcc_add (t_irc_server *, int, unsigned long, int, char *, int,
                           char *, char *, unsigned long);
extern void dcc_send_request (t_irc_server *, int, char *, char *);
extern void dcc_chat_sendf (t_irc_dcc *, char *, ...);
extern void dcc_file_send_fork (t_irc_dcc *);
extern void dcc_file_recv_fork (t_irc_dcc *);
extern void dcc_handle ();
extern void dcc_end ();
extern void dcc_print_log ();

/* IRC display (irc-diplay.c) */

extern void irc_find_context (t_irc_server *, t_irc_channel *,
                              t_gui_window **, t_gui_buffer **);
extern void irc_display_prefix (t_irc_server *, t_gui_buffer *, char *);
extern void irc_display_nick (t_gui_buffer *, t_irc_nick *, char *, int,
                              int, int, int);
extern void irc_display_away (t_irc_server *, char *, char *);
extern void irc_display_mode (t_irc_server *, t_gui_buffer *, char *, char *,
                              char, char *, char *, char *, char *);
extern void irc_display_server (t_irc_server *ptr_server);

/* IRC commands issued by user (irc-send.c) */

extern void irc_login (t_irc_server *);
extern int irc_cmd_send_admin (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_ame (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_amsg (t_irc_server *, t_irc_channel *, char *);
extern void irc_send_away (t_irc_server *, char *);
extern int irc_cmd_send_away (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_ban (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_ctcp (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_cycle (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_dehalfop (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_deop (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_devoice (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_die (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_halfop (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_info (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_invite (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_ison (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_join (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_kick (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_kickban (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_kill (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_links (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_list (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_lusers (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_me (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_mode (t_irc_server *, t_irc_channel *, char *);
extern void irc_send_mode_nicks (t_irc_server *, char *, char *, char *, int, char **);
extern int irc_cmd_send_motd (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_msg (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_names (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_nick (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_notice (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_op (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_oper (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_part (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_ping (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_pong (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_query (t_irc_server *, t_irc_channel *, char *);
extern void irc_send_quit_server (t_irc_server *, char *);
extern int irc_cmd_send_quit (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_quote (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_rehash (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_restart (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_service (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_servlist (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_squery (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_squit (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_stats (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_summon (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_time (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_topic (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_trace (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_unban (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_userhost (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_users (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_version (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_voice (t_irc_server *, t_irc_channel *, int, char **);
extern int irc_cmd_send_wallops (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_who (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_whois (t_irc_server *, t_irc_channel *, char *);
extern int irc_cmd_send_whowas (t_irc_server *, t_irc_channel *, char *);

/* IRC commands executed when received from server (irc-recv.c) */

extern int irc_is_highlight (char *, char *);
extern int irc_recv_command (t_irc_server *, char *, char *, char *, char *);
extern int irc_cmd_recv_error (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_invite (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_join (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_kick (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_kill (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_mode (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_nick (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_notice (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_part (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_ping (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_pong (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_privmsg (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_quit (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_server_mode_reason (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_server_msg (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_server_reply (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_topic (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_wallops (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_001 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_221 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_301 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_302 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_303 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_305 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_306 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_whois_nick_msg (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_310 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_311 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_312 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_314 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_315 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_317 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_319 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_321 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_322 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_323 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_324 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_329 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_331 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_332 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_333 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_338 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_341 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_344 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_345 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_348 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_349 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_351 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_352 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_353 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_365 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_366 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_367 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_368 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_378 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_433 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_438 (t_irc_server *, char *, char *, char *);
extern int irc_cmd_recv_671 (t_irc_server *, char *, char *, char *);

/* ignore functions (irc-ignore.c) */

extern int ignore_check (char *, char *, char *, char *);
extern t_irc_ignore *ignore_add (char *, char *, char *, char *);
extern t_irc_ignore *ignore_add_from_config (char *);
extern void ignore_free_all ();
extern int ignore_search_free (char *, char *, char *, char *);
extern int ignore_search_free_by_number (int);
extern void ignore_print_log ();

#endif /* irc.h */
