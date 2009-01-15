/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_SERVER_H
#define __WEECHAT_JABBER_SERVER_H 1

#include <sys/time.h>
#include <regex.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include <iksemel.h>

enum t_jabber_server_option
{
    JABBER_SERVER_OPTION_USERNAME = 0,  /* username on server                */
    JABBER_SERVER_OPTION_SERVER,        /* server hostname/IP                */
    JABBER_SERVER_OPTION_PROXY,         /* proxy used for server (optional)  */
    JABBER_SERVER_OPTION_IPV6,          /* use IPv6 protocol                 */
    JABBER_SERVER_OPTION_TLS,           /* use TLS cryptographic protocol    */
    JABBER_SERVER_OPTION_SASL,          /* use SASL for auth                 */
    JABBER_SERVER_OPTION_RESOURCE,      /* resource                          */
    JABBER_SERVER_OPTION_PASSWORD,      /* password                          */
    JABBER_SERVER_OPTION_LOCAL_ALIAS,   /* local alias                       */
    JABBER_SERVER_OPTION_AUTOCONNECT,   /* autoconnect to server at startup  */
    JABBER_SERVER_OPTION_AUTORECONNECT, /* autoreconnect when disconnected   */
    JABBER_SERVER_OPTION_AUTORECONNECT_DELAY, /* delay before next reconnect */
    JABBER_SERVER_OPTION_LOCAL_HOSTNAME,/* custom local hostname             */
    JABBER_SERVER_OPTION_COMMAND,       /* command to run once connected     */
    JABBER_SERVER_OPTION_COMMAND_DELAY, /* delay after execution of command  */
    JABBER_SERVER_OPTION_AUTOJOIN,      /* MUCs to automatically join        */
    JABBER_SERVER_OPTION_AUTOREJOIN,    /* auto rejoin MUCs when kicked      */
    /* number of server options */
    JABBER_SERVER_NUM_OPTIONS,
};

#define JABBER_SERVER_OPTION_BOOLEAN(__server, __index)                         \
    ((!weechat_config_option_is_null(__server->options[__index])) ?             \
     weechat_config_boolean(__server->options[__index]) :                       \
     ((!weechat_config_option_is_null(jabber_config_server_default[__index])) ? \
      weechat_config_boolean(jabber_config_server_default[__index])             \
      : weechat_config_boolean_default(jabber_config_server_default[__index])))

#define JABBER_SERVER_OPTION_INTEGER(__server, __index)                         \
    ((!weechat_config_option_is_null(__server->options[__index])) ?             \
     weechat_config_integer(__server->options[__index]) :                       \
     ((!weechat_config_option_is_null(jabber_config_server_default[__index])) ? \
      weechat_config_integer(jabber_config_server_default[__index])             \
      : weechat_config_integer_default(jabber_config_server_default[__index])))

#define JABBER_SERVER_OPTION_STRING(__server, __index)                          \
    ((!weechat_config_option_is_null(__server->options[__index])) ?             \
     weechat_config_string(__server->options[__index]) :                        \
     ((!weechat_config_option_is_null(jabber_config_server_default[__index])) ? \
      weechat_config_string(jabber_config_server_default[__index])              \
      : weechat_config_string_default(jabber_config_server_default[__index])))

#define JABBER_SERVER_DEFAULT_PORT          5222
#define JABBER_SERVER_DEFAULT_RESOURCE      "WeeChat"

#define jabber_server_sendf_queued(server, fmt, argz...)        \
    if (server)                                                 \
    {                                                           \
        server->queue_msg = 1;                                  \
        jabber_server_sendf (server, fmt, ##argz);              \
        server->queue_msg = 0;                                  \
    }

struct t_jabber_server
{
    /* user choices */
    char *name;                     /* internal name of server               */
    struct t_config_option *options[JABBER_SERVER_NUM_OPTIONS];
    
    /* internal vars */
    int temp_server;                /* temporary server (not saved)          */
    int reloading_from_config;      /* 1 if reloading from config file       */
    int reloaded_from_config;       /* 1 if reloaded from config file        */
    char *address;                  /* address from "server" option          */
    int port;                       /* port from "server" option             */
    char *current_ip;               /* current IP address                    */
    int sock;                       /* socket for server (IPv4 or IPv6)      */
    iksparser *iks_parser;          /* parser for libiksemel                 */
    char *iks_id_string;            /* string with id (user@server/resource) */
    iksid *iks_id;                  /* id for libiksemel                     */
    char *iks_server_name;          /* server name for libiksemel            */
    char *iks_password;             /* password for libiksemel               */
    iksfilter *iks_filter;          /* filter for libiksemel                 */
    iks *iks_roster;                /* jabber roster (buddy list)            */
    int iks_features;               /* stream features                       */
    int iks_authorized;             /* authorized by jabber server           */
    struct t_hook *hook_connect;    /* connection hook                       */
    struct t_hook *hook_fd;         /* hook for server socket                */
    int is_connected;               /* 1 if WeeChat is connected to server   */
    int tls_connected;              /* = 1 if connected with TLS             */
#ifdef HAVE_GNUTLS
    gnutls_session_t gnutls_sess;   /* gnutls session (only if TLS is used)  */
#endif
    time_t reconnect_start;         /* this time + delay = reconnect time    */
    time_t command_time;            /* this time + command_delay = time to   */
                                    /* autojoin MUCs                         */
    int reconnect_join;             /* 1 if opened MUCs to rejoin            */
    int disable_autojoin;           /* 1 if user asked to not autojoin MUCs  */
    int is_away;                    /* 1 is user is marked as away           */
    char *away_message;             /* away message, NULL if not away        */
    time_t away_time;               /* time() when user marking as away      */
    int lag;                        /* lag (in milliseconds)                 */
    struct timeval lag_check_time;  /* last time lag was checked (ping sent) */
    time_t lag_next_check;          /* time for next check                   */
    struct t_gui_buffer *buffer;    /* GUI buffer allocated for server       */
    char *buffer_as_string;         /* used to return buffer info            */
    int buddies_count;              /* # buddies in roster                   */
    struct t_jabber_buddy *buddies; /* buddies in roster                     */
    struct t_jabber_buddy *last_buddy;   /* last buddy in roster             */
    struct t_jabber_muc *mucs;           /* MUCs opened on server            */
    struct t_jabber_muc *last_muc;       /* last opened MUC on server        */
    struct t_jabber_server *prev_server; /* link to previous server          */
    struct t_jabber_server *next_server; /* link to next server              */
};

/* Jabber messages */

struct t_jabber_message
{
    struct t_jabber_server *server; /* server pointer for received msg       */
    char *data;                     /* message content                       */
    struct t_jabber_message *next_message; /* link to next message           */
};

extern struct t_jabber_server *jabber_servers;
extern struct t_jabber_server *jabber_current_server;
#ifdef HAVE_GNUTLS
extern const int gnutls_cert_type_prio[];
extern const int gnutls_prot_prio[];
#endif
extern struct t_jabber_message *jabber_recv_msgq, *jabber_msgq_last_msg;
extern char *jabber_server_option_string[];
extern char *jabber_server_option_default[];

extern int jabber_server_valid (struct t_jabber_server *server);
extern int jabber_server_search_option (const char *option_name);
extern struct t_jabber_server *jabber_server_search (const char *server_name);
extern int jabber_server_get_muc_count (struct t_jabber_server *server);
extern int jabber_server_get_pv_count (struct t_jabber_server *server);
extern char *jabber_server_get_name_without_port (const char *name);
extern const char *jabber_server_get_local_name (struct t_jabber_server *server);
extern void jabber_server_set_server (struct t_jabber_server *server,
                                      const char *address);
extern void jabber_server_set_nicks (struct t_jabber_server *server,
                                     const char *nicks);
extern void jabber_server_buffer_set_highlight_words (struct t_gui_buffer *buffer);
extern struct t_jabber_server *jabber_server_alloc (const char *name);
extern void jabber_server_set_buffer_title (struct t_jabber_server *server);
extern struct t_gui_buffer *jabber_server_create_buffer (struct t_jabber_server *server,
                                                         int all_servers);
extern void jabber_server_set_current_server (struct t_jabber_server *server);
extern int jabber_server_iks_transport_connect_async (iksparser *parser,
                                                      void **socketptr,
                                                      const char *server,
                                                      const char *server_name,
                                                      int port,
                                                      void *notify_data,
                                                      iksAsyncNotify *notify_func);
extern int jabber_server_iks_transport_send (void *socket, const char *data,
                                             size_t len);
extern int jabber_server_iks_transport_recv (void *socket, char *buffer,
                                             size_t buf_len, int timeout);
extern void jabber_server_iks_transport_close (void *socket);
extern int jabber_server_connect (struct t_jabber_server *server);
extern void jabber_server_auto_connect ();
extern void jabber_server_disconnect (struct t_jabber_server *server,
                                      int reconnect);
extern void jabber_server_disconnect_all ();
extern void jabber_server_free_data (struct t_jabber_server *server);
extern void jabber_server_free (struct t_jabber_server *server);
extern void jabber_server_free_all ();
extern struct t_jabber_server *jabber_server_copy (struct t_jabber_server *server,
                                                   const char *new_name);
extern int jabber_server_rename (struct t_jabber_server *server,
                                 const char *new_server_name);
extern int jabber_server_recv_cb (void *arg_server);
extern int jabber_server_timer_cb (void *data);
extern int jabber_server_add_to_infolist (struct t_infolist *infolist,
                                          struct t_jabber_server *server);
extern void jabber_server_print_log ();

#endif /* jabber-server.h */
