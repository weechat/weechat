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


#ifndef __WEECHAT_IRC_SERVER_H
#define __WEECHAT_IRC_SERVER_H 1

#include <regex.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include "irc-channel.h"
#include "../../core/hook.h"
#include "../../gui/gui.h"

#ifndef NI_MAXHOST
#define NI_MAXHOST 256
#endif

#define IRC_SERVER_DEFAULT_PORT 6667
#define IRC_SERVER_DEFAULT_PREFIXES_LIST "@%+~&!-"

#define irc_server_sendf_queued(server, fmt, argz...) \
    if (server) \
    { \
        server->queue_msg = 1; \
        irc_server_sendf (server, fmt, ##argz); \
        server->queue_msg = 0; \
    }

/* output queue of messages to server (for sending slowly to server) */

typedef struct t_irc_outqueue t_irc_outqueue;

struct t_irc_outqueue
{
    char *message_before_mod;       /* message before any modifier           */
    char *message_after_mod;        /* message after modifier(s)             */
    int modified;                   /* message was modified by modifier(s)   */
    t_irc_outqueue *next_outqueue;  /* pointer to next message in queue      */
    t_irc_outqueue *prev_outqueue;  /* pointer to previous message in queue  */
};

typedef struct t_irc_server t_irc_server;

struct t_irc_server
{
    /* user choices */
    char *name;                     /* internal name of server               */
    int autoconnect;                /* = 1 if auto connect at startup        */
    int autoreconnect;              /* = 1 if auto reco when disconnected    */
    int autoreconnect_delay;        /* delay before trying again reconnect   */
    int temp_server;                /* server is temporary (not saved!)      */
    char *address;                  /* address of server (IP or name)        */
    int port;                       /* port for server (6667 by default)     */
    int ipv6;                       /* use IPv6 protocol                     */
    int ssl;                        /* SSL protocol                          */
    char *password;                 /* password for server                   */
    char *nick1;                    /* first nickname for the server         */
    char *nick2;                    /* alternate nickname                    */
    char *nick3;                    /* 2nd alternate nickname                */
    char *username;                 /* user name                             */
    char *realname;                 /* real name                             */
    char *hostname;                 /* custom hostname                       */
    char *command;                  /* command to run once connected         */
    int command_delay;              /* delay after execution of command      */
    char *autojoin;                 /* channels to automatically join        */
    int autorejoin;                 /* auto rejoin channels when kicked      */
    char *notify_levels;            /* channels notify levels                */
    
    /* internal vars */
    pid_t child_pid;                /* pid of child process (connecting)     */
    int child_read;                 /* to read into child pipe               */
    int child_write;                /* to write into child pipe              */
    int sock;                       /* socket for server (IPv4 or IPv6)      */
    t_weechat_hook *hook_fd;        /* hook for server socket or child pipe  */
    int is_connected;               /* 1 if WeeChat is connected to server   */
    int ssl_connected;              /* = 1 if connected with SSL             */
#ifdef HAVE_GNUTLS
    gnutls_session gnutls_sess;     /* gnutls session (only if SSL is used)  */
#endif
    char *unterminated_message;     /* beginning of a message in input buf   */
    char *nick;                     /* current nickname                      */
    char *nick_modes;               /* nick modes                            */
    char *prefix;                   /* nick prefix allowed (from msg 005)    */
    time_t reconnect_start;         /* this time + delay = reconnect time    */
    time_t command_time;            /* this time + command_delay = time to   */
                                    /* autojoin channels                     */
    int reconnect_join;             /* 1 if channels opened to rejoin        */
    int disable_autojoin;           /* 1 if user asked to not autojoin chans */
    int is_away;                    /* 1 is user is marked as away           */
    char *away_message;             /* away message, NULL if not away        */
    time_t away_time;               /* time() when user marking as away      */
    int lag;                        /* lag (in milliseconds)                 */
    struct timeval lag_check_time;  /* last time lag was checked (ping sent) */
    time_t lag_next_check;          /* time for next check                   */
    regex_t *cmd_list_regexp;       /* compiled Regular Expression for /list */
    int queue_msg;                  /* set to 1 when queue (out) is required */
    time_t last_user_message;       /* time of last user message (anti flood)*/
    t_irc_outqueue *outqueue;       /* queue for outgoing user messages      */
    t_irc_outqueue *last_outqueue;  /* last outgoing user message            */
    t_gui_buffer *buffer;           /* GUI buffer allocated for server       */
    t_irc_channel *channels;        /* opened channels on server             */
    t_irc_channel *last_channel;    /* last opened channal on server         */
    t_irc_server *prev_server;      /* link to previous server               */
    t_irc_server *next_server;      /* link to next server                   */
};

/* IRC messages */

typedef struct t_irc_message t_irc_message;

struct t_irc_message
{
    t_irc_server *server;           /* server pointer for received msg       */
    char *data;                     /* message content                       */
    t_irc_message *next_message;    /* link to next message                  */
};

extern t_irc_server *irc_servers;
#ifdef HAVE_GNUTLS
extern const int gnutls_cert_type_prio[];
extern const int gnutls_prot_prio[];
#endif
extern t_irc_message *irc_recv_msgq, *irc_msgq_last_msg;

#endif /* irc-server.h */
