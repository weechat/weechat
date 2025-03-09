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

#ifndef WEECHAT_PLUGIN_RELAY_CLIENT_H
#define WEECHAT_PLUGIN_RELAY_CLIENT_H

#include <time.h>

#include <gnutls/gnutls.h>

struct t_relay_server;
struct t_relay_http_request;

/* type of data exchanged with client */

enum t_relay_client_data_type
{
    RELAY_CLIENT_DATA_TEXT_LINE = 0,   /* text messages (on one line)       */
    RELAY_CLIENT_DATA_BINARY,          /* binary messages                   */
    RELAY_CLIENT_DATA_HTTP,            /* HTTP messages (mostly text)       */
    RELAY_CLIENT_DATA_TEXT_MULTILINE,  /* multi-line text (eg: JSON)        */
    /* number of data types */
    RELAY_NUM_CLIENT_DATA_TYPES,
};

/* websocket status */

enum t_relay_client_websocket_status
{
    RELAY_CLIENT_WEBSOCKET_NOT_USED = 0, /* no webseocket or not yet init.  */
    RELAY_CLIENT_WEBSOCKET_INITIALIZING, /* websocket used, initializing    */
    RELAY_CLIENT_WEBSOCKET_READY,        /* websocket used, ready           */
    /* number of websocket status */
    RELAY_NUM_CLIENT_WEBSOCKET_STATUS,
};

/* fake send function (for tests) */

typedef void (t_relay_fake_send_func)(void *client,
                                      const char *data, int data_size);

/* output queue of messages to client */

struct t_relay_client_outqueue
{
    char *data;                         /* data to send                     */
    int data_size;                      /* number of bytes                  */
    int raw_msg_type[2];                /* msgs types                       */
    int raw_flags[2];                   /* flags for raw messages           */
    char *raw_message[2];               /* msgs for raw buffer (can be NULL)*/
    int raw_size[2];                    /* size (in bytes) of raw messages  */
    struct t_relay_client_outqueue *next_outqueue; /* next msg in queue     */
    struct t_relay_client_outqueue *prev_outqueue; /* prev msg in queue     */
};

/* relay client */

struct t_relay_client
{
    int id;                            /* unique id (diff. for each client) */
    char *desc;                        /* description, used for display     */
    int sock;                          /* socket for connection             */
    int server_port;                   /* port used for connection          */
    int tls;                           /* 1 if TLS is enabled               */
    gnutls_session_t gnutls_sess;      /* gnutls session (only if TLS used) */
    t_relay_fake_send_func *fake_send_func; /* function called for fake send*/
                                       /* (used in tests only)              */
    struct t_hook *hook_timer_handshake; /* timer for doing gnutls handshake*/
    int gnutls_handshake_ok;           /* 1 if handshake was done and OK    */
    enum t_relay_client_websocket_status websocket; /* websocket status     */
    struct t_relay_websocket_deflate *ws_deflate; /* websocket deflate data */
    struct t_relay_http_request *http_req; /* HTTP request                  */
    char *address;                     /* string with IP address            */
    char *real_ip;                     /* real IP (X-Real-IP HTTP header)   */
    enum t_relay_status status;        /* status (connecting, active,..)    */
    enum t_relay_protocol protocol;    /* protocol (irc,..)                 */
    char *protocol_string;             /* example: "ipv6.tls.irc.libera"    */
    char *protocol_args;               /* arguments used for protocol       */
                                       /* example: server for irc protocol  */
    char *nonce;                       /* nonce used in salt of hashed pwd  */
    int password_hash_algo;            /* password hash algo (negotiated)   */
    time_t listen_start_time;          /* when listening started            */
    time_t start_time;                 /* time of client connection         */
    time_t end_time;                   /* time of client disconnection      */
    struct t_hook *hook_fd;            /* hook for socket or child pipe     */
    struct t_hook *hook_timer_send;    /* timer to quickly flush outqueue   */
    time_t last_activity;              /* time of last byte received/sent   */
    unsigned long long bytes_recv;     /* bytes received from client        */
    unsigned long long bytes_sent;     /* bytes sent to client              */
    enum t_relay_client_data_type recv_data_type; /* type recv from client  */
    enum t_relay_client_data_type send_data_type; /* type sent to client    */
    char *partial_ws_frame;            /* part. binary websocket frame recv */
    int partial_ws_frame_size;         /* size of partial websocket frame   */
    char *partial_message;             /* partial text message received     */
    void *protocol_data;               /* data depending on protocol used   */
    struct t_relay_client_outqueue *outqueue; /* queue for outgoing msgs    */
    struct t_relay_client_outqueue *last_outqueue; /* last outgoing msg     */
    struct t_relay_client *prev_client;/* link to previous client           */
    struct t_relay_client *next_client;/* link to next client               */
};

extern struct t_relay_client *relay_clients;
extern struct t_relay_client *last_relay_client;
extern int relay_client_count;

extern int relay_client_valid (struct t_relay_client *client);
extern struct t_relay_client *relay_client_search_by_number (int number);
extern struct t_relay_client *relay_client_search_by_id (int id);
extern int relay_client_count_active_by_port (int server_port);
extern void relay_client_set_desc (struct t_relay_client *client);
extern void relay_client_recv_buffer (struct t_relay_client *client,
                                      const char *buffer, int buffer_size);
extern int relay_client_recv_cb (const void *pointer, void *data, int fd);
extern int relay_client_send (struct t_relay_client *client,
                              enum t_relay_msg_type msg_type,
                              const char *data,
                              int data_size, const char *message_raw_buffer);
extern void relay_client_timer (void);
extern struct t_relay_client *relay_client_new (int sock, const char *address,
                                                struct t_relay_server *server);
extern struct t_relay_client *relay_client_new_with_infolist (struct t_infolist *infolist);
extern void relay_client_set_status (struct t_relay_client *client,
                                     enum t_relay_status status);
extern void relay_client_free (struct t_relay_client *client);
extern void relay_client_free_all (void);
extern void relay_client_disconnect (struct t_relay_client *client);
extern void relay_client_disconnect_all (void);
extern int relay_client_add_to_infolist (struct t_infolist *infolist,
                                         struct t_relay_client *client,
                                         int force_disconnected_state);
extern void relay_client_print_log (void);

#endif /* WEECHAT_PLUGIN_RELAY_CLIENT_H */
