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

#ifndef __WEECHAT_IRC_SERVER_H
#define __WEECHAT_IRC_SERVER_H 1

#include <sys/time.h>
#include <regex.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST 256
#endif

enum t_irc_server_option
{
    IRC_SERVER_OPTION_ADDRESSES = 0, /* server addresses (IP/name with port) */
    IRC_SERVER_OPTION_PROXY,         /* proxy used for server (optional)     */
    IRC_SERVER_OPTION_IPV6,          /* use IPv6 protocol                    */
    IRC_SERVER_OPTION_SSL,           /* SSL protocol                         */
    IRC_SERVER_OPTION_SSL_CERT,      /* client ssl certificate file          */
    IRC_SERVER_OPTION_SSL_PRIORITIES, /* gnutls priorities                   */
    IRC_SERVER_OPTION_SSL_DHKEY_SIZE, /* Diffie Hellman key size             */
    IRC_SERVER_OPTION_SSL_VERIFY,    /* check if the connection is trusted   */
    IRC_SERVER_OPTION_PASSWORD,      /* password for server                  */
    IRC_SERVER_OPTION_CAPABILITIES,  /* client capabilities to enable        */
    IRC_SERVER_OPTION_SASL_MECHANISM,/* mechanism for SASL authentication    */
    IRC_SERVER_OPTION_SASL_USERNAME, /* username for SASL authentication     */
    IRC_SERVER_OPTION_SASL_PASSWORD, /* password for SASL authentication     */
    IRC_SERVER_OPTION_SASL_TIMEOUT,  /* timeout for SASL authentication      */
    IRC_SERVER_OPTION_AUTOCONNECT,   /* autoconnect to server at startup     */
    IRC_SERVER_OPTION_AUTORECONNECT, /* autoreconnect when disconnected      */
    IRC_SERVER_OPTION_AUTORECONNECT_DELAY, /* delay before trying again reco */
    IRC_SERVER_OPTION_NICKS,         /* nicknames (comma separated list)     */
    IRC_SERVER_OPTION_USERNAME,      /* user name                            */
    IRC_SERVER_OPTION_REALNAME,      /* real name                            */
    IRC_SERVER_OPTION_LOCAL_HOSTNAME,/* custom local hostname                */
    IRC_SERVER_OPTION_COMMAND,       /* command to run once connected        */
    IRC_SERVER_OPTION_COMMAND_DELAY, /* delay after execution of command     */
    IRC_SERVER_OPTION_AUTOJOIN,      /* channels to automatically join       */
    IRC_SERVER_OPTION_AUTOREJOIN,    /* auto rejoin channels when kicked     */
    IRC_SERVER_OPTION_AUTOREJOIN_DELAY,     /* delay before auto rejoin      */
    IRC_SERVER_OPTION_CONNECTION_TIMEOUT,   /* timeout for connection        */
    IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_HIGH, /* anti-flood (high priority)    */
    IRC_SERVER_OPTION_ANTI_FLOOD_PRIO_LOW,  /* anti-flood (low priority)     */
    IRC_SERVER_OPTION_AWAY_CHECK,           /* delay between away checks     */
    IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS, /* max nicks for away check      */
    IRC_SERVER_OPTION_DEFAULT_MSG_PART,     /* default part message          */
    IRC_SERVER_OPTION_DEFAULT_MSG_QUIT,     /* default quit message          */
    IRC_SERVER_OPTION_NOTIFY,               /* notify list                   */
    /* number of server options */
    IRC_SERVER_NUM_OPTIONS,
};

#define IRC_SERVER_OPTION_BOOLEAN(__server, __index)                          \
    ((!weechat_config_option_is_null(__server->options[__index])) ?           \
     weechat_config_boolean(__server->options[__index]) :                     \
     ((!weechat_config_option_is_null(irc_config_server_default[__index])) ?  \
      weechat_config_boolean(irc_config_server_default[__index])              \
      : weechat_config_boolean_default(irc_config_server_default[__index])))

#define IRC_SERVER_OPTION_INTEGER(__server, __index)                          \
    ((!weechat_config_option_is_null(__server->options[__index])) ?           \
     weechat_config_integer(__server->options[__index]) :                     \
     ((!weechat_config_option_is_null(irc_config_server_default[__index])) ?  \
      weechat_config_integer(irc_config_server_default[__index])              \
      : weechat_config_integer_default(irc_config_server_default[__index])))

#define IRC_SERVER_OPTION_STRING(__server, __index)                           \
    ((!weechat_config_option_is_null(__server->options[__index])) ?           \
     weechat_config_string(__server->options[__index]) :                      \
     ((!weechat_config_option_is_null(irc_config_server_default[__index])) ?  \
      weechat_config_string(irc_config_server_default[__index])               \
      : weechat_config_string_default(irc_config_server_default[__index])))

#define IRC_SERVER_DEFAULT_PORT  6667
#define IRC_SERVER_DEFAULT_NICKS "weechat1,weechat2,weechat3,weechat4,weechat5"

/* number of queues for sending messages */
#define IRC_SERVER_NUM_OUTQUEUES_PRIO 2

/* flags for irc_server_sendf() */
#define IRC_SERVER_SEND_OUTQ_PRIO_HIGH   1
#define IRC_SERVER_SEND_OUTQ_PRIO_LOW    2
#define IRC_SERVER_SEND_RETURN_HASHTABLE 4

/* casemapping (string comparisons for nicks/channels) */
enum t_irc_server_casemapping
{
    IRC_SERVER_CASEMAPPING_RFC1459 = 0,    /* A-Z [ \ ] ^ ==> a-z { | } ~    */
    IRC_SERVER_CASEMAPPING_STRICT_RFC1459, /* A-Z [ \ ]   ==> a-z { | }      */
    IRC_SERVER_CASEMAPPING_ASCII,          /* A-Z         ==> a-z            */
    /* number of casemapping */
    IRC_SERVER_NUM_CASEMAPPING,
};

/* output queue of messages to server (for sending slowly to server) */

struct t_irc_outqueue
{
    char *command;                        /* IRC command                     */
    char *message_before_mod;             /* msg before any modifier         */
    char *message_after_mod;              /* msg after modifier(s)           */
    int modified;                         /* msg was modified by modifier(s) */
    char *tags;                           /* tags (used by Relay plugin)     */
    struct t_irc_redirect *redirect;      /* command redirection             */
    struct t_irc_outqueue *next_outqueue; /* link to next msg in queue       */
    struct t_irc_outqueue *prev_outqueue; /* link to prev msg in queue       */
};

struct t_irc_server
{
    /* user choices */
    char *name;                             /* internal name of server       */
    struct t_config_option *options[IRC_SERVER_NUM_OPTIONS];

    /* internal vars */
    int temp_server;                /* temporary server (not saved)          */
    int reloading_from_config;      /* 1 if reloading from config file       */
    int reloaded_from_config;       /* 1 if reloaded from config file        */
    int addresses_count;            /* number of addresses                   */
    char **addresses_array;         /* addresses (after split)               */
    int *ports_array;               /* ports for addresses                   */
    int index_current_address;      /* current address index in array        */
    char *current_address;          /* current address                       */
    char *current_ip;               /* current IP address                    */
    int current_port;               /* current port                          */
    int sock;                       /* socket for server (IPv4 or IPv6)      */
    struct t_hook *hook_connect;    /* connection hook                       */
    struct t_hook *hook_fd;         /* hook for server socket                */
    struct t_hook *hook_timer_connection; /* timer for connection            */
    struct t_hook *hook_timer_sasl; /* timer for SASL authentication         */
    int is_connected;               /* 1 if WeeChat is connected to server   */
    int ssl_connected;              /* = 1 if connected with SSL             */
#ifdef HAVE_GNUTLS
    gnutls_session_t gnutls_sess;   /* gnutls session (only if SSL is used)  */
    gnutls_x509_crt_t tls_cert;     /* certificate used if ssl_cert is set   */
    gnutls_x509_privkey_t tls_cert_key; /* key used if ssl_cert is set       */
#endif
    char *unterminated_message;     /* beginning of a message in input buf   */
    int nicks_count;                /* number of nicknames                   */
    char **nicks_array;             /* nicknames (after split)               */
    int nick_first_tried;           /* first nick tried in list of nicks     */
                                    /* when (re-)connecting to server        */
    char *nick;                     /* current nickname                      */
    char *nick_modes;               /* nick modes                            */
    char *isupport;                 /* copy of message 005 (ISUPPORT)        */
    char *prefix_modes;             /* prefix modes from msg 005 (eg "ohv")  */
    char *prefix_chars;             /* prefix chars from msg 005 (eg "@%+")  */
    int nick_max_length;            /* max lenth of nick (from msg 005)      */
    int casemapping;                /* casemapping from msg 005              */
    char *chantypes;                /* chantypes from msg 005 (eg "&#")      */
    char *chanmodes;                /* chanmodes from msg 005                */
                                    /* (eg "beI,k,l,imnpstaqr")              */
    int reconnect_delay;            /* current reconnect delay (growing)     */
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
    time_t lag_last_refresh;        /* last refresh of lag item              */
    regex_t *cmd_list_regexp;       /* compiled Regular Expression for /list */
    time_t last_user_message;       /* time of last user message (anti flood)*/
    time_t last_away_check;         /* time of last away check on server     */
    struct t_irc_outqueue *outqueue[2];      /* queue for outgoing messages  */
                                             /* with 2 priorities (high/low) */
    struct t_irc_outqueue *last_outqueue[2]; /* last outgoing message        */
    struct t_irc_redirect *redirects;        /* command redirections         */
    struct t_irc_redirect *last_redirect;    /* last command redirection     */
    struct t_irc_notify *notify_list;        /* list of notify               */
    struct t_irc_notify *last_notify;        /* last notify                  */
    struct t_hashtable *manual_joins;        /* manual joins pending         */
    struct t_gui_buffer *buffer;          /* GUI buffer allocated for server */
    char *buffer_as_string;               /* used to return buffer info      */
    struct t_irc_channel *channels;       /* opened channels on server       */
    struct t_irc_channel *last_channel;   /* last opened channel on server   */
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
extern char *irc_server_option_string[];
extern char *irc_server_option_default[];

extern int irc_server_valid (struct t_irc_server *server);
extern struct t_irc_server *irc_server_search (const char *server_name);
extern struct t_irc_server *irc_server_casesearch (const char *server_name);
extern int irc_server_search_option (const char *option_name);
extern int irc_server_search_casemapping (const char *casemapping);
extern int irc_server_strcasecmp (struct t_irc_server *server,
                                  const char *string1, const char *string2);
extern int irc_server_strncasecmp (struct t_irc_server *server,
                                   const char *string1, const char *string2,
                                   int max);
extern int irc_server_sasl_enabled (struct t_irc_server *server);
extern char *irc_server_get_name_without_port (const char *name);
extern void irc_server_set_addresses (struct t_irc_server *server,
                                      const char *addresses);
extern void irc_server_set_nicks (struct t_irc_server *server, const char *nicks);
extern void irc_server_set_nick (struct t_irc_server *server, const char *nick);
extern int irc_server_get_nick_index (struct t_irc_server *server);
extern const char *irc_server_get_isupport_value (struct t_irc_server *server,
                                                  const char *feature);
extern void irc_server_set_prefix_modes_chars (struct t_irc_server *server,
                                               const char *prefix);
extern const char *irc_server_get_prefix_modes (struct t_irc_server *server);
extern const char *irc_server_get_prefix_chars (struct t_irc_server *server);
extern int irc_server_get_prefix_mode_index (struct t_irc_server *server,
                                             char mode);
extern int irc_server_get_prefix_char_index (struct t_irc_server *server,
                                             char prefix_char);
extern char irc_server_get_prefix_mode_for_char (struct t_irc_server *server,
                                                 char prefix_char);
extern char irc_server_get_prefix_char_for_mode (struct t_irc_server *server,
                                                 char mode);
extern const char *irc_server_get_chanmodes (struct t_irc_server *server);
extern struct t_irc_server *irc_server_alloc (const char *name);
extern struct t_irc_server *irc_server_alloc_with_url (const char *irc_url);
extern void irc_server_apply_command_line_options (struct t_irc_server *server,
                                                   int argc, char **argv);
extern void irc_server_free_all ();
extern struct t_irc_server *irc_server_copy (struct t_irc_server *server,
                                             const char *new_name);
extern int irc_server_rename (struct t_irc_server *server, const char *new_name);
extern void irc_server_send_signal (struct t_irc_server *server,
                                    const char *signal, const char *command,
                                    const char *full_message,
                                    const char *tags);
extern void irc_server_set_send_default_tags (const char *tags);
extern struct t_hashtable *irc_server_sendf (struct t_irc_server *server,
                                             int flags,
                                             const char *tags,
                                             const char *format, ...);
extern void irc_server_msgq_add_buffer (struct t_irc_server *server,
                                        const char *buffer);
extern void irc_server_msgq_flush ();
extern void irc_server_set_buffer_title (struct t_irc_server *server);
extern struct t_gui_buffer *irc_server_create_buffer (struct t_irc_server *server);
extern int irc_server_connect (struct t_irc_server *server);
extern void irc_server_auto_connect (int auto_connect);
extern void irc_server_autojoin_channels ();
extern int irc_server_recv_cb (void *data, int fd);
extern int irc_server_timer_sasl_cb (void *data, int remaining_calls);
extern int irc_server_timer_cb (void *data, int remaining_calls);
extern void irc_server_outqueue_free_all (struct t_irc_server *server,
                                          int priority);
extern int irc_server_get_channel_count (struct t_irc_server *server);
extern int irc_server_get_pv_count (struct t_irc_server *server);
extern void irc_server_set_away (struct t_irc_server *server, const char *nick,
                                 int is_away);
extern void irc_server_remove_away (struct t_irc_server *server);
extern void irc_server_check_away (struct t_irc_server *server);
extern void irc_server_switch_address (struct t_irc_server *server,
                                       int connection);
extern void irc_server_disconnect (struct t_irc_server *server,
                                   int switch_address, int reconnect);
extern void irc_server_disconnect_all ();
extern void irc_server_free (struct t_irc_server *server);
extern int irc_server_xfer_send_ready_cb (void *data, const char *signal,
                                          const char *type_data, void *signal_data);
extern int irc_server_xfer_resume_ready_cb (void *data, const char *signal,
                                            const char *type_data, void *signal_data);
extern int irc_server_xfer_send_accept_resume_cb (void *data, const char *signal,
                                                  const char *type_data,
                                                  void *signal_data);
extern struct t_hdata *irc_server_hdata_server_cb (void *data,
                                                   const char *hdata_name);
extern int irc_server_add_to_infolist (struct t_infolist *infolist,
                                       struct t_irc_server *server);
extern void irc_server_print_log ();

#endif /* __WEECHAT_IRC_SERVER_H */
