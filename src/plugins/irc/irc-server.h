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


#ifndef __WEECHAT_IRC_SERVER_H
#define __WEECHAT_IRC_SERVER_H 1

#include <regex.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST 256
#endif

#define IRC_SERVER_DEFAULT_PORT          6667
#define IRC_SERVER_DEFAULT_PREFIXES_LIST "@%+~&!-"
#define IRC_SERVER_DEFAULT_NICKS         "weechat1,weechat2,weechat3,"  \
    "weechat4,weechat5"

#define irc_server_sendf_queued(server, fmt, argz...)   \
    if (server)                                         \
    {                                                   \
        server->queue_msg = 1;                          \
        irc_server_sendf (server, fmt, ##argz);         \
        server->queue_msg = 0;                          \
    }

/* output queue of messages to server (for sending slowly to server) */

struct t_irc_outqueue
{
    char *message_before_mod;             /* msg before any modifier         */
    char *message_after_mod;              /* msg after modifier(s)           */
    int modified;                         /* msg was modified by modifier(s) */
    struct t_irc_outqueue *next_outqueue; /* link to next msg in queue       */
    struct t_irc_outqueue *prev_outqueue; /* link to prev msg in queue       */
};

struct t_irc_server
{
    /* user choices */
    char *name;                     /* internal name of server               */
    int autoconnect;                /* = 1 if auto connect at startup        */
    int autoreconnect;              /* = 1 if auto reco when disconnected    */
    int autoreconnect_delay;        /* delay before trying again reconnect   */
    int temp_server;                /* server is temporary (not saved!)      */
    char *addresses;                /* server addresses (IP/name with port)  */
    int ipv6;                       /* use IPv6 protocol                     */
    int ssl;                        /* SSL protocol                          */
    char *password;                 /* password for server                   */
    char *nicks;                    /* nicknames as one string               */
    char *username;                 /* user name                             */
    char *realname;                 /* real name                             */
    char *hostname;                 /* custom hostname                       */
    char *command;                  /* command to run once connected         */
    int command_delay;              /* delay after execution of command      */
    char *autojoin;                 /* channels to automatically join        */
    int autorejoin;                 /* auto rejoin channels when kicked      */
    char *notify_levels;            /* channels notify levels                */
    
    /* internal vars */
    int reloaded_from_config;       /* 1 if reloaded from config file        */
    int addresses_count;            /* number of addresses                   */
    char **addresses_array;         /* exploded addresses                    */
    int *ports_array;               /* ports for addresses                   */
    int current_address;            /* current address index in array        */
    pid_t child_pid;                /* pid of child process (connecting)     */
    int child_read;                 /* to read into child pipe               */
    int child_write;                /* to write into child pipe              */
    int sock;                       /* socket for server (IPv4 or IPv6)      */
    struct t_hook *hook_fd;         /* hook for server socket or child pipe  */
    int is_connected;               /* 1 if WeeChat is connected to server   */
    int ssl_connected;              /* = 1 if connected with SSL             */
#ifdef HAVE_GNUTLS
    gnutls_session gnutls_sess;     /* gnutls session (only if SSL is used)  */
#endif
    char *unterminated_message;     /* beginning of a message in input buf   */
    int nicks_count;                /* number of nicknames                   */
    char **nicks_array;             /* exploded nicknames                    */
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
    struct t_irc_outqueue *outqueue;      /* queue for outgoing user msgs    */
    struct t_irc_outqueue *last_outqueue; /* last outgoing user message      */
    struct t_gui_buffer *buffer;          /* GUI buffer allocated for server */
    struct t_irc_channel *channels;       /* opened channels on server       */
    struct t_irc_channel *last_channel;   /* last opened channal on server   */
    struct t_irc_server *prev_server;     /* link to previous server         */
    struct t_irc_server *next_server;     /* link to next server             */
};

/* IRC messages */

struct t_irc_message
{
    struct t_irc_server *server;        /* server pointer for received msg   */
    char *data;                         /* message content                   */
    struct t_irc_message *next_message; /* link to next message              */
};

extern struct t_irc_server *irc_servers;
#ifdef HAVE_GNUTLS
extern const int gnutls_cert_type_prio[];
extern const int gnutls_prot_prio[];
#endif
extern struct t_irc_message *irc_recv_msgq, *irc_msgq_last_msg;


extern void irc_server_set_addresses (struct t_irc_server *server,
                                      char *addresses);
extern void irc_server_set_nicks (struct t_irc_server *server, char *nicks);
extern void irc_server_set_with_option (struct t_irc_server *server,
                                        int index_option,
                                        struct t_config_option *option);
extern void irc_server_init (struct t_irc_server *server);
extern struct t_irc_server *irc_server_alloc (char *name);
extern int irc_server_alloc_with_url (char *irc_url);
extern struct t_irc_server *irc_server_new (char *name, int autoconnect,
                                            int autoreconnect,
                                            int autoreconnect_delay,
                                            int temp_server, char *addresses,
                                            int ipv6, int ssl,
                                            char *password, char *nicks,
                                            char *username, char *realname,
                                            char *hostname, char *command,
                                            int command_delay, char *autojoin,
                                            int autorejoin,
                                            char *notify_levels);
extern struct t_irc_server *irc_server_duplicate (struct t_irc_server *server,
                                                  char *new_name);
extern int irc_server_rename (struct t_irc_server *server, char *new_name);
extern void irc_server_set_nick (struct t_irc_server *server, char *nick);
extern struct t_irc_server *irc_server_search (char *server_name);
extern void irc_server_free_all ();
extern int irc_server_connect (struct t_irc_server *server,
                               int disable_autojoin);
extern void irc_server_auto_connect (int auto_connect, int temp_server);
extern void irc_server_autojoin_channels ();
extern int irc_server_timer_cb (void *data);
extern void irc_server_sendf (struct t_irc_server *server, char *format, ...);
extern void irc_server_outqueue_free_all (struct t_irc_server *server);
extern int irc_server_get_channel_count (struct t_irc_server *server);
extern int irc_server_get_pv_count (struct t_irc_server *server);
extern void irc_server_set_away (struct t_irc_server *server, char *nick,
                                 int is_away);
extern void irc_server_remove_away ();
extern void irc_server_check_away ();
extern void irc_server_reconnect (struct t_irc_server *server);
extern void irc_server_disconnect (struct t_irc_server *server, int reconnect);
extern void irc_server_disconnect_all ();
extern void irc_server_free (struct t_irc_server *server);
extern void irc_server_free_data (struct t_irc_server *server);
extern int irc_server_xfer_send_ready_cb (void *data, char *signal,
                                          char *type_data, void *signal_data);
extern int irc_server_xfer_resume_ready_cb (void *data, char *signal,
                                            char *type_data, void *signal_data);
extern int irc_server_xfer_send_accept_resume_cb (void *data, char *signal,
                                                  char *type_data,
                                                  void *signal_data);
extern void irc_server_print_log ();

#endif /* irc-server.h */
