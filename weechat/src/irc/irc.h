/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __WEECHAT_IRC_H
#define __WEECHAT_IRC_H 1

#include "../gui/gui.h"

#define PREFIX_SERVER    "-@-"
#define PREFIX_INFO      "-=-"
#define PREFIX_ACTION_ME "-*-"
#define PREFIX_JOIN      "-->"
#define PREFIX_PART      "<--"
#define PREFIX_QUIT      "<--"
#define PREFIX_ERROR     "=!="
#define PREFIX_PLUGIN    "-P-"

#define CHANNEL_PREFIX "#&+!"

#define NUM_CHANNEL_MODES       7
#define CHANNEL_MODE_INVITE     0
#define CHANNEL_MODE_KEY        1
#define CHANNEL_MODE_LIMIT      2
#define CHANNEL_MODE_MODERATED  3
#define CHANNEL_MODE_NO_MSG_OUT 4
#define CHANNEL_MODE_SECRET     5
#define CHANNEL_MODE_TOPIC      6
#define SET_CHANNEL_MODE(channel, set, mode) \
    if (set) \
        channel->modes[mode] = channel_modes[mode]; \
    else \
        channel->modes[mode] = ' ';

#define DEFAULT_IRC_PORT 6667

/* nick types */

typedef struct t_irc_nick t_irc_nick;

struct t_irc_nick
{
    char *nick;                     /* nickname                             */
    int is_op;                      /* operator privileges?                 */
    int is_halfop;                  /* half operaor privileges?             */
    int has_voice;                  /* nick has voice?                      */
    int color;                      /* color for nickname                   */
    t_irc_nick *prev_nick;          /* link to previous nick on the channel */
    t_irc_nick *next_nick;          /* link to next nick on the channel     */
};

/* channel types */

typedef struct t_irc_channel t_irc_channel;

#define CHAT_UNKNOWN -1
#define CHAT_CHANNEL 0
#define CHAT_PRIVATE 1

struct t_irc_channel
{
    int type;                       /* channel type                         */
    char *name;                     /* name of channel (exemple: "#abc")    */
    char *topic;                    /* topic of channel (host for private)  */
    char modes[NUM_CHANNEL_MODES+1];/* channel modes                        */
    int limit;                      /* user limit (0 is limit not set)      */
    char *key;                      /* channel key (NULL if no key is set)  */
    t_irc_nick *nicks;              /* nicks on the channel                 */
    t_irc_nick *last_nick;          /* last nick on the channel             */
    t_gui_view *view;               /* GUI view allocated for channel       */
    t_irc_channel *prev_channel;    /* link to previous channel             */
    t_irc_channel *next_channel;    /* link to next channel                 */
};

/* server types */

typedef struct t_irc_server t_irc_server;

struct t_irc_server
{
    /* user choices */
    char *name;                     /* name of server (only for display)    */
    int autoconnect;                /* = 1 if auto connect at startup       */
    int command_line;               /* server was given on command line     */
    char *address;                  /* address of server (IP or name)       */
    int port;                       /* port for server (6667 by default)    */
    char *password;                 /* password for server                  */
    char *nick1;                    /* first nickname for the server        */
    char *nick2;                    /* alternate nickname                   */
    char *nick3;                    /* 2nd alternate nickname               */
    char *username;                 /* user name                            */
    char *realname;                 /* real name                            */
    char *command;					/* command to run once connected        */
    char *autojoin;					/* channels to automatically join       */
    
    /* internal vars */
    char *nick;                     /* current nickname                     */
    int is_connected;               /* 1 if WeeChat is connected to server  */
    int sock4;                      /* socket for server                    */
    int is_away;                    /* 1 is user is marker as away          */
    int server_read;                /* pipe for reading server data         */
    int server_write;               /* pipe for sending data to server      */
    t_gui_view *view;               /* GUI view allocated for server        */
    t_irc_channel *channels;        /* opened channels on server            */
    t_irc_channel *last_channel;    /* last opened channal on server        */
    t_irc_server *prev_server;      /* link to previous server              */
    t_irc_server *next_server;      /* link to next server                  */
};

/* irc commands */

typedef struct t_irc_command t_irc_command;

struct t_irc_command
{
    char *command_name;             /* command name (internal or IRC cmd)   */
    char *command_description;      /* command description                  */
    char *arguments;                /* command parameters                   */
    char *arguments_description;    /* parameters description               */
    int min_arg, max_arg;           /* min & max number of parameters       */
    int need_connection;            /* = 1 if cmd needs server connection   */
    int (*cmd_function_args)(t_irc_server *, int, char **);
                                    /* function called when user enters cmd */
    int (*cmd_function_1arg)(t_irc_server *, char *);
                                    /* function called when user enters cmd */
    int (*recv_function)(t_irc_server *, char *, char *);
                                    /* function called when cmd is received */
};

typedef struct t_irc_message t_irc_message;

struct t_irc_message
{
    t_irc_server *server;           /* server pointer for received msg      */
    char *data;                     /* message content                      */
    t_irc_message *next_message;    /* link to next message                 */
};

extern t_irc_command irc_commands[];
extern t_irc_server *irc_servers, *current_irc_server;
extern t_irc_message *recv_msgq, *msgq_last_msg;
extern t_irc_channel *current_channel;
extern char *channel_modes;

/* server functions (irc-server.c) */

extern void server_init (t_irc_server *);
extern int server_init_with_url (char *, t_irc_server *);
extern t_irc_server *server_alloc ();
extern void server_destroy (t_irc_server *);
extern void server_free (t_irc_server *);
extern void server_free_all ();
extern t_irc_server *server_new (char *, int, int, char *, int, char *, char *,
                                 char *, char *, char *, char *, char *, char *);
extern int server_send (t_irc_server *, char *, int);
extern void server_sendf (t_irc_server *, char *, ...);
extern void server_recv (t_irc_server *);
extern int server_connect ();
extern void server_auto_connect (int);
extern void server_disconnect (t_irc_server *);
extern void server_disconnect_all ();
extern t_irc_server *server_search (char *);
extern int server_get_number_connected ();
extern int server_name_already_exists (char *);

/* channel functions (irc-channel.c) */

extern t_irc_channel *channel_new (t_irc_server *, int, char *, int);
extern void channel_free (t_irc_server *, t_irc_channel *);
extern void channel_free_all (t_irc_server *);
extern t_irc_channel *channel_search (t_irc_server *, char *);
extern int string_is_channel (char *);

/* nick functions (irc-nick.c) */

extern t_irc_nick *nick_new (t_irc_channel *, char *, int, int, int);
extern void nick_resort (t_irc_channel *, t_irc_nick *);
extern void nick_change (t_irc_channel *, t_irc_nick *, char *);
extern void nick_free (t_irc_channel *, t_irc_nick *);
extern void nick_free_all (t_irc_channel *);
extern t_irc_nick *nick_search (t_irc_channel *, char *);
extern void nick_count (t_irc_channel *, int *, int *, int *, int *, int *);
extern int nick_get_max_length (t_irc_channel *);

/* DCC functions (irc-dcc.c) */

extern void dcc_send ();

/* IRC display (irc-diplay.c) */

extern void irc_display_prefix (/*@null@*/ t_gui_view *, char *);
extern void irc_display_nick (t_gui_view *, t_irc_nick *, int, int, int, int);
extern void irc_display_mode (t_gui_view *, char *, char, char *, char *,
                              char *, char *);

/* IRC protocol (irc-commands.c) */

extern int irc_recv_command (t_irc_server *, char *, char *, char *, char *);
extern void irc_login (t_irc_server *);
/* IRC commands issued by user */
extern int irc_cmd_send_admin (t_irc_server *, char *);
extern int irc_cmd_send_away (t_irc_server *, char *);
extern int irc_cmd_send_ctcp (t_irc_server *, char *);
extern int irc_cmd_send_dcc (t_irc_server *, char *);
extern int irc_cmd_send_deop (t_irc_server *, int, char **);
extern int irc_cmd_send_devoice (t_irc_server *, int, char **);
extern int irc_cmd_send_die (t_irc_server *, char *);
extern int irc_cmd_send_info (t_irc_server *, char *);
extern int irc_cmd_send_invite (t_irc_server *, char *);
extern int irc_cmd_send_ison (t_irc_server *, char *);
extern int irc_cmd_send_join (t_irc_server *, char *);
extern int irc_cmd_send_kick (t_irc_server *, char *);
extern int irc_cmd_send_kill (t_irc_server *, char *);
extern int irc_cmd_send_links (t_irc_server *, char *);
extern int irc_cmd_send_list (t_irc_server *, char *);
extern int irc_cmd_send_lusers (t_irc_server *, char *);
extern int irc_cmd_send_me (t_irc_server *, char *);
extern int irc_cmd_send_mode (t_irc_server *, char *);
extern int irc_cmd_send_motd (t_irc_server *, char *);
extern int irc_cmd_send_msg (t_irc_server *, char *);
extern int irc_cmd_send_names (t_irc_server *, char *);
extern int irc_cmd_send_nick (t_irc_server *, int, char **);
extern int irc_cmd_send_notice (t_irc_server *, char *);
extern int irc_cmd_send_op (t_irc_server *, int, char **);
extern int irc_cmd_send_oper (t_irc_server *, char *);
extern int irc_cmd_send_part (t_irc_server *, char *);
extern int irc_cmd_send_ping (t_irc_server *, char *);
extern int irc_cmd_send_pong (t_irc_server *, char *);
extern int irc_cmd_send_query (t_irc_server *, char *);
extern int irc_cmd_send_quit (t_irc_server *, char *);
extern int irc_cmd_send_quote (t_irc_server *, char *);
extern int irc_cmd_send_rehash (t_irc_server *, char *);
extern int irc_cmd_send_restart (t_irc_server *, char *);
extern int irc_cmd_send_service (t_irc_server *, char *);
extern int irc_cmd_send_servlist (t_irc_server *, char *);
extern int irc_cmd_send_squery (t_irc_server *, char *);
extern int irc_cmd_send_squit (t_irc_server *, char *);
extern int irc_cmd_send_stats (t_irc_server *, char *);
extern int irc_cmd_send_summon (t_irc_server *, char *);
extern int irc_cmd_send_time (t_irc_server *, char *);
extern int irc_cmd_send_topic (t_irc_server *, char *);
extern int irc_cmd_send_trace (t_irc_server *, char *);
extern int irc_cmd_send_userhost (t_irc_server *, char *);
extern int irc_cmd_send_users (t_irc_server *, char *);
extern int irc_cmd_send_version (t_irc_server *, char *);
extern int irc_cmd_send_voice (t_irc_server *, int, char **);
extern int irc_cmd_send_wallops (t_irc_server *, char *);
extern int irc_cmd_send_who (t_irc_server *, char *);
extern int irc_cmd_send_whois (t_irc_server *, char *);
extern int irc_cmd_send_whowas (t_irc_server *, char *);
/* IRC commands executed when received from server */
extern int irc_cmd_recv_error (t_irc_server *, char *, char *);
extern int irc_cmd_recv_join (t_irc_server *, char *, char *);
extern int irc_cmd_recv_kick (t_irc_server *, char *, char *);
extern int irc_cmd_recv_mode (t_irc_server *, char *, char *);
extern int irc_cmd_recv_nick (t_irc_server *, char *, char *);
extern int irc_cmd_recv_notice (t_irc_server *, char *, char *);
extern int irc_cmd_recv_part (t_irc_server *, char *, char *);
extern int irc_cmd_recv_ping (t_irc_server *, char *, char *);
extern int irc_cmd_recv_privmsg (t_irc_server *, char *, char *);
extern int irc_cmd_recv_quit (t_irc_server *, char *, char *);
extern int irc_cmd_recv_server_msg (t_irc_server *, char *, char *);
extern int irc_cmd_recv_server_reply (t_irc_server *, char *, char *);
extern int irc_cmd_recv_topic (t_irc_server *, char *, char *);
extern int irc_cmd_recv_001 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_004 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_301 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_302 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_303 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_305 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_306 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_311 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_312 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_313 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_314 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_317 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_318 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_319 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_320 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_321 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_322 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_323 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_324 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_329 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_331 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_332 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_333 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_351 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_352 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_353 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_365 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_366 (t_irc_server *, char *, char *);
extern int irc_cmd_recv_433 (t_irc_server *, char *, char *);

#endif /* irc.h */
