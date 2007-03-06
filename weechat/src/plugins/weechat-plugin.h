/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* This header is designed to be distributed with WeeChat plugins */

#ifndef __WEECHAT_WEECHAT_PLUGIN_H
#define __WEECHAT_WEECHAT_PLUGIN_H 1

#include <sys/types.h>

/* return codes for init function and handlers */
#define PLUGIN_RC_KO                 -1 /* function/handler failed          */
#define PLUGIN_RC_OK                 0  /* function/handler ok              */

/* return codes specific to message handlers: messages can be discarded for
   WeeChat, for plugins, or both */
#define PLUGIN_RC_OK_IGNORE_WEECHAT  1  /* ignore WeeChat for this message  */
#define PLUGIN_RC_OK_IGNORE_PLUGINS  2  /* ignore other plugins for this msg*/
#define PLUGIN_RC_OK_IGNORE_ALL      (PLUGIN_RC_OK_IGNORE_WEECHAT \
                                     | PLUGIN_RC_OK_IGNORE_PLUGINS)
                                        /* ignore WeeChat and other plugins */
#define PLUGIN_RC_OK_WITH_HIGHLIGHT  4  /* ok and ask for highlight         */
                                        /* (for message handler only)       */

#define WEECHAT_IRC_COLOR_WHITE        0
#define WEECHAT_IRC_COLOR_BLACK        1
#define WEECHAT_IRC_COLOR_BLUE         2
#define WEECHAT_IRC_COLOR_GREEN        3
#define WEECHAT_IRC_COLOR_LIGHTRED     4
#define WEECHAT_IRC_COLOR_RED          5
#define WEECHAT_IRC_COLOR_MAGENTA      6
#define WEECHAT_IRC_COLOR_BROWN        7
#define WEECHAT_IRC_COLOR_YELLOW       8
#define WEECHAT_IRC_COLOR_LIGHTGREEN   9
#define WEECHAT_IRC_COLOR_CYAN         10
#define WEECHAT_IRC_COLOR_LIGHTCYAN    11
#define WEECHAT_IRC_COLOR_LIGHTBLUE    12
#define WEECHAT_IRC_COLOR_LIGHTMAGENTA 13
#define WEECHAT_IRC_COLOR_GRAY         14
#define WEECHAT_IRC_COLOR_LIGHTGRAY    15

typedef struct t_plugin_dcc_info t_plugin_dcc_info;

struct t_plugin_dcc_info
{
    char *server;                   /* irc server                           */
    char *channel;                  /* irc channel (for DCC chat only)      */
    int type;                       /* DCC type (send or receive)           */
    int status;                     /* DCC status (waiting, sending, ..)    */
    time_t start_time;              /* the time when DCC started            */
    time_t start_transfer;          /* the time when DCC transfer started   */
    unsigned long addr;             /* IP address                           */
    int port;                       /* port                                 */
    char *nick;                     /* remote nick                          */
    char *filename;                 /* filename (given by sender)           */
    char *local_filename;           /* local filename (with path)           */
    int filename_suffix;            /* suffix (.1 for ex) if renaming file  */
    unsigned long size;             /* file size                            */
    unsigned long pos;              /* number of bytes received/sent        */
    unsigned long start_resume;     /* start of resume (in bytes)           */
    unsigned long bytes_per_sec;    /* bytes per second                     */
    t_plugin_dcc_info *prev_dcc;    /* link to previous dcc file/chat       */
    t_plugin_dcc_info *next_dcc;    /* link to next dcc file/chat           */
};

typedef struct t_plugin_server_info t_plugin_server_info;

struct t_plugin_server_info
{
    char *name;                     /* name of server (only for display)    */
    int autoconnect;                /* = 1 if auto connect at startup       */
    int autoreconnect;              /* = 1 if auto reco when disconnected   */
    int autoreconnect_delay;        /* delay before trying again reconnect  */
    int command_line;               /* server was given on command line     */
    char *address;                  /* address of server (IP or name)       */
    int port;                       /* port for server (6667 by default)    */
    int ipv6;                       /* use IPv6 protocol                    */
    int ssl;                        /* SSL protocol                         */
    char *password;                 /* password for server                  */
    char *nick1;                    /* first nickname for the server        */
    char *nick2;                    /* alternate nickname                   */
    char *nick3;                    /* 2nd alternate nickname               */
    char *username;                 /* user name                            */
    char *realname;                 /* real name                            */
    char *command;                  /* command to run once connected        */
    int command_delay;              /* delay after execution of command     */
    char *autojoin;                 /* channels to automatically join       */
    int autorejoin;                 /* auto rejoin channels when kicked     */
    char *notify_levels;            /* channels notify levels               */
    int is_connected;               /* 1 if WeeChat is connected to server  */
    int ssl_connected;              /* = 1 if connected with SSL            */
    char *nick;                     /* current nickname                     */
    char *nick_modes;               /* nick modes                           */
    int is_away;                    /* 1 is user is marker as away          */
    time_t away_time;               /* time() when user marking as away     */
    int lag;                        /* lag (in milliseconds)                */
    t_plugin_server_info *prev_server; /* link to previous server info      */
    t_plugin_server_info *next_server; /* link to next server info          */
};

typedef struct t_plugin_channel_info t_plugin_channel_info;

struct t_plugin_channel_info
{
    int type;                       /* channel type                         */
    char *name;                     /* name of channel (exemple: "#abc")    */
    char *topic;                    /* topic of channel (host for private)  */
    char *modes;                    /* channel modes                        */
    int limit;                      /* user limit (0 is limit not set)      */
    char *key;                      /* channel key (NULL if no key is set)  */
    int nicks_count;                /* # nicks on channel (0 if dcc/pv)     */
    t_plugin_channel_info *prev_channel; /* link to previous channel info   */
    t_plugin_channel_info *next_channel; /* link to next channel info       */
};

typedef struct t_plugin_nick_info t_plugin_nick_info;

struct t_plugin_nick_info
{
    char *nick;                     /* nickname                             */
    int flags;                      /* chanowner/chanadmin (unrealircd),    */
    char *host;                     /* hostname                             */
                                    /* op, halfop, voice, away              */
    t_plugin_nick_info *prev_nick;  /* link to previous nick                */
    t_plugin_nick_info *next_nick;  /* link to next nick                    */
};

typedef struct t_plugin_window_info t_plugin_window_info;

struct t_plugin_window_info
{
    int win_x, win_y;               /* position of window                   */
    int win_width, win_height;      /* window geometry                      */
    int win_width_pct;              /* % of width (compared to parent win)  */
    int win_height_pct;             /* % of height (compared to parent win) */
    int num_buffer;                 /* # of displayed buffer                */
    t_plugin_window_info *prev_window; /* link to previous window           */
    t_plugin_window_info *next_window; /* link to next window               */
};

typedef struct t_plugin_buffer_info t_plugin_buffer_info;

struct t_plugin_buffer_info
{
    int type;                       /* buffer type (0=standard,1=dcc,2=raw) */
    int number;                     /* buffer number                        */
    int num_displayed;              /* number of windows displaying buffer  */
    char *server_name;              /* server name for buffer (may be NULL) */
    char *channel_name;             /* channel name for buffer (may be NULL)*/
    int notify_level;               /* notify level for buffer              */
    char *log_filename;             /* log filename (NULL is disabled)      */
    t_plugin_buffer_info *prev_buffer; /* link to previous buffer           */
    t_plugin_buffer_info *next_buffer; /* link to next buffer               */
};

typedef struct t_plugin_buffer_line t_plugin_buffer_line;

struct t_plugin_buffer_line
{
    time_t date;                     /* date                                */
    char *nick;                      /* nick                                */
    char *data;                      /* line data                           */
    t_plugin_buffer_line *prev_line; /* link to previous line               */
    t_plugin_buffer_line *next_line; /* link to next line                   */
};

typedef struct t_weechat_plugin t_weechat_plugin;

/* handlers */

typedef int (t_plugin_handler_func) (t_weechat_plugin *, int, char **, char *, void *);

typedef enum t_plugin_handler_type t_plugin_handler_type;

enum t_plugin_handler_type
{
    PLUGIN_HANDLER_MESSAGE = 0,     /* IRC message handler                  */
    PLUGIN_HANDLER_COMMAND,         /* command handler                      */
    PLUGIN_HANDLER_TIMER,           /* timer handler                        */
    PLUGIN_HANDLER_KEYBOARD,        /* keyboard handler                     */
    PLUGIN_HANDLER_EVENT            /* event handler                        */
};

typedef struct t_plugin_handler t_plugin_handler;

struct t_plugin_handler
{
    t_plugin_handler_type type;     /* handler type                         */
    
    /* data for message handler */
    char *irc_command;              /* name of IRC command (PRIVMSG, ..)    */
    
    /* data for command handler */
    char *command;                  /* name of command (without first '/')  */
    char *description;              /* (for /help) short cmd description    */
    char *arguments;                /* (for /help) command arguments        */
    char *arguments_description;    /* (for /help) args long description    */
    char *completion_template;      /* template for completion              */
    
    /* data for timer handler */
    int interval;                   /* interval between two calls to fct    */
    int remaining;                  /* seconds remaining before next call   */

    /* data for event handler */
    char *event;                    /* event to catch                       */
    
    /* data common to all handlers */
    t_plugin_handler_func *handler; /* pointer to handler                   */
    char *handler_args;             /* arguments sent to handler            */
    void *handler_pointer;          /* pointer sent to handler              */
    
    /* for internal use */
    int running;                    /* 1 if currently running               */
                                    /* (used to prevent circular call)      */
    t_plugin_handler *prev_handler; /* link to previous handler             */
    t_plugin_handler *next_handler; /* link to next handler                 */
};

/* modifiers */

typedef char * (t_plugin_modifier_func) (t_weechat_plugin *, int, char **, char *, void *);

typedef enum t_plugin_modifier_type t_plugin_modifier_type;

enum t_plugin_modifier_type
{
    PLUGIN_MODIFIER_IRC_IN = 0,     /* incoming IRC msg (server > user)     */
    PLUGIN_MODIFIER_IRC_USER,       /* outgoing IRC msg (user > server)     */
                                    /* after user input (before 'out' mod.) */
    PLUGIN_MODIFIER_IRC_OUT         /* outgoing IRC msg (user > server)     */
                                    /* immediately before sending to server */
};

#define PLUGIN_MODIFIER_IRC_IN_STR   "irc_in"
#define PLUGIN_MODIFIER_IRC_USER_STR "irc_user"
#define PLUGIN_MODIFIER_IRC_OUT_STR  "irc_out"

typedef struct t_plugin_modifier t_plugin_modifier;

struct t_plugin_modifier
{
    t_plugin_modifier_type type;    /* modifier type                        */

    /* data for IRC modifier */
    char *command;                  /* IRC command                          */
    
    /* data common to all modifiers */
    t_plugin_modifier_func *modifier; /* pointer to modifier                */
    char *modifier_args;            /* arguments sent to modifier           */
    void *modifier_pointer;         /* pointer sent to modifier             */
    
    /* for internal use */
    int running;                    /* 1 if currently running               */
                                    /* (used to prevent circular call)      */
    t_plugin_modifier *prev_modifier; /* link to previous modifier          */
    t_plugin_modifier *next_modifier; /* link to next modifier              */
};

/* plugin, a WeeChat plugin, which is a dynamic library */

struct t_weechat_plugin
{
    /* plugin variables */
    char *filename;                 /* name of plugin on disk               */
    void *handle;                   /* handle of plugin (given by dlopen)   */
    char *name;                     /* plugin name                          */
    char *description;              /* plugin description                   */
    char *version;                  /* plugin version                       */
    char *charset;                  /* charset used by plugin               */
    
    /* plugin handlers */
    t_plugin_handler *handlers;     /* pointer to first handler             */
    t_plugin_handler *last_handler; /* pointer to last handler              */

    /* plugin modifiers */
    t_plugin_modifier *modifiers;     /* pointer to first modifier          */
    t_plugin_modifier *last_modifier; /* pointer to last modifier           */
    
    /* links to previous/next plugins */
    t_weechat_plugin *prev_plugin;  /* link to previous plugin              */
    t_weechat_plugin *next_plugin;  /* link to next plugin                  */
    
    /* plugin functions (interface) */
    
    /* IMPORTANT NOTE for WeeChat developers: always add new interface functions
       at the END of functions, for keeping backward compatibility with
       existing plugins */
    
    int (*ascii_strcasecmp) (t_weechat_plugin *, char *, char *);
    int (*ascii_strncasecmp) (t_weechat_plugin *, char *, char *, int);
    char **(*explode_string) (t_weechat_plugin *, char *, char *, int, int *);
    void (*free_exploded_string) (t_weechat_plugin *, char **);
    int (*mkdir_home) (t_weechat_plugin *, char *);
    void (*exec_on_files) (t_weechat_plugin *, char *,
                           int (*)(t_weechat_plugin *, char *));
    
    void (*print) (t_weechat_plugin *, char *, char *, char *, ...);
    void (*print_server) (t_weechat_plugin *, char *, ...);
    void (*print_infobar) (t_weechat_plugin *, int, char *, ...);
    void (*infobar_remove) (t_weechat_plugin *, int);
    
    t_plugin_handler *(*msg_handler_add) (t_weechat_plugin *, char *,
                                          t_plugin_handler_func *,
                                          char *, void *);
    t_plugin_handler *(*cmd_handler_add) (t_weechat_plugin *, char *,
                                          char *, char *, char *,
                                          char *,
                                          t_plugin_handler_func *,
                                          char *, void *);
    t_plugin_handler *(*timer_handler_add) (t_weechat_plugin *, int,
                                            t_plugin_handler_func *,
                                            char *, void *);
    t_plugin_handler *(*keyboard_handler_add) (t_weechat_plugin *,
                                               t_plugin_handler_func *,
                                               char *, void *);
    t_plugin_handler *(*event_handler_add) (t_weechat_plugin *, char *,
                                            t_plugin_handler_func *,
                                            char *, void *);
    void (*handler_remove) (t_weechat_plugin *, t_plugin_handler *);
    void (*handler_remove_all) (t_weechat_plugin *);

    t_plugin_modifier *(*modifier_add) (t_weechat_plugin *, char *, char *,
                                        t_plugin_modifier_func *,
                                        char *, void *);
    void (*modifier_remove) (t_weechat_plugin *, t_plugin_modifier *);
    void (*modifier_remove_all) (t_weechat_plugin *);
    
    void (*exec_command) (t_weechat_plugin *, char *, char *, char *);
    char *(*get_info) (t_weechat_plugin *, char *, char *);
    t_plugin_dcc_info *(*get_dcc_info) (t_weechat_plugin *);
    void (*free_dcc_info) (t_weechat_plugin *, t_plugin_dcc_info *);
    char *(*get_config) (t_weechat_plugin *, char *);
    int (*set_config) (t_weechat_plugin *, char *, char *);
    char *(*get_plugin_config) (t_weechat_plugin *, char *);
    int (*set_plugin_config) (t_weechat_plugin *, char *, char *);
    t_plugin_server_info *(*get_server_info) (t_weechat_plugin *);
    void (*free_server_info) (t_weechat_plugin *, t_plugin_server_info *);
    t_plugin_channel_info *(*get_channel_info) (t_weechat_plugin *, char *);
    void (*free_channel_info) (t_weechat_plugin *, t_plugin_channel_info *);
    t_plugin_nick_info *(*get_nick_info) (t_weechat_plugin *, char*, char*);
    void (*free_nick_info) (t_weechat_plugin *, t_plugin_nick_info *);
    
    void (*log) (t_weechat_plugin *, char *, char *, char *, ...);
    
    void (*input_color) (t_weechat_plugin *, int, int, int);

    int (*get_irc_color) (t_weechat_plugin *, char *);

    t_plugin_window_info *(*get_window_info) (t_weechat_plugin *);
    void (*free_window_info) (t_weechat_plugin *, t_plugin_window_info *);
    t_plugin_buffer_info *(*get_buffer_info) (t_weechat_plugin *);
    void (*free_buffer_info) (t_weechat_plugin *, t_plugin_buffer_info *);
    t_plugin_buffer_line *(*get_buffer_data) (t_weechat_plugin *, char *, char *);
    void (*free_buffer_data) (t_weechat_plugin *, t_plugin_buffer_line *);

    void (*set_charset) (t_weechat_plugin *, char *);
    char *(*iconv_to_internal) (t_weechat_plugin *, char *, char *);
    char *(*iconv_from_internal) (t_weechat_plugin *, char *, char *);
    
    /* WeeChat developers: ALWAYS add new functions at the end */
};

/* general useful functions */
extern int weechat_ascii_strcasecmp (t_weechat_plugin *,char *, char *);
extern int weechat_ascii_strncasecmp (t_weechat_plugin *,char *, char *, int);
extern char **weechat_explode_string (t_weechat_plugin *, char *, char *, int, int *);
extern void weechat_free_exploded_string (t_weechat_plugin *, char **);
extern int weechat_plugin_mkdir_home (t_weechat_plugin *, char *);
extern void weechat_plugin_exec_on_files (t_weechat_plugin *, char *,
                                          int (*)(t_weechat_plugin *, char *));

/* display functions */
extern void weechat_plugin_print (t_weechat_plugin *, char *, char *, char *, ...);
extern void weechat_plugin_print_server (t_weechat_plugin *, char *, ...);
extern void weechat_plugin_print_infobar (t_weechat_plugin *, int, char *, ...);
extern void weechat_plugin_infobar_remove (t_weechat_plugin *, int);

/* log functions */
extern void weechat_plugin_log (t_weechat_plugin *, char *, char *, char *, ...);

/* handler functions */
extern t_plugin_handler *weechat_plugin_msg_handler_add (t_weechat_plugin *, char *,
                                                         t_plugin_handler_func *,
                                                         char *, void *);
extern t_plugin_handler *weechat_plugin_cmd_handler_add (t_weechat_plugin *, char *,
                                                         char *, char *, char *,
                                                         char *,
                                                         t_plugin_handler_func *,
                                                         char *, void *);
extern t_plugin_handler *weechat_plugin_timer_handler_add (t_weechat_plugin *, int,
                                                           t_plugin_handler_func *,
                                                           char *, void *);
extern t_plugin_handler *weechat_plugin_keyboard_handler_add (t_weechat_plugin *,
                                                              t_plugin_handler_func *,
                                                              char *, void *);
extern t_plugin_handler *weechat_plugin_event_handler_add (t_weechat_plugin *, char *,
                                                           t_plugin_handler_func *,
                                                           char *, void *);
extern void weechat_plugin_handler_remove (t_weechat_plugin *, t_plugin_handler *);
extern void weechat_plugin_handler_remove_all (t_weechat_plugin *);

/* modifier functions */
extern t_plugin_modifier *weechat_plugin_modifier_add (t_weechat_plugin *,
                                                       char *, char *,
                                                       t_plugin_modifier_func *,
                                                       char *, void *);
extern void weechat_plugin_modifier_remove (t_weechat_plugin *, t_plugin_modifier *);
extern void weechat_plugin_modifier_remove_all (t_weechat_plugin *);

/* other functions */
extern void weechat_plugin_exec_command (t_weechat_plugin *, char *, char *, char *);
extern char *weechat_plugin_get_info (t_weechat_plugin *, char *, char *);
extern t_plugin_dcc_info *weechat_plugin_get_dcc_info (t_weechat_plugin *);
extern void weechat_plugin_free_dcc_info (t_weechat_plugin *, t_plugin_dcc_info *);
extern char *weechat_plugin_get_config (t_weechat_plugin *, char *);
extern int weechat_plugin_set_config (t_weechat_plugin *, char *, char *);
extern char *weechat_plugin_get_plugin_config (t_weechat_plugin *, char *);
extern int weechat_plugin_set_plugin_config (t_weechat_plugin *, char *, char *);
extern t_plugin_server_info *weechat_plugin_get_server_info (t_weechat_plugin *);
extern void weechat_plugin_free_server_info (t_weechat_plugin *, t_plugin_server_info *);
extern t_plugin_channel_info *weechat_plugin_get_channel_info (t_weechat_plugin *, char *);
extern void weechat_plugin_free_channel_info (t_weechat_plugin *, t_plugin_channel_info *);
extern t_plugin_nick_info *weechat_plugin_get_nick_info (t_weechat_plugin *, char *, char *);
extern void weechat_plugin_free_nick_info (t_weechat_plugin *, t_plugin_nick_info *);
extern void weechat_plugin_input_color (t_weechat_plugin *, int, int, int);
extern int weechat_plugin_get_irc_color (t_weechat_plugin *, char *);
extern t_plugin_window_info *weechat_plugin_get_window_info (t_weechat_plugin *);
extern void weechat_plugin_free_window_info (t_weechat_plugin *, t_plugin_window_info *);
extern t_plugin_buffer_info *weechat_plugin_get_buffer_info (t_weechat_plugin *);
extern void weechat_plugin_free_buffer_info (t_weechat_plugin *, t_plugin_buffer_info *);
extern t_plugin_buffer_line *weechat_plugin_get_buffer_data (t_weechat_plugin *, char *, char *);
extern void weechat_plugin_free_buffer_data (t_weechat_plugin *, t_plugin_buffer_line *);

/* iconv functions */
extern void weechat_plugin_set_charset (t_weechat_plugin *, char *);
extern char *weechat_plugin_iconv_to_internal (t_weechat_plugin *, char *, char *);
extern char *weechat_plugin_iconv_from_internal (t_weechat_plugin *, char *, char *);

#endif /* weechat-plugin.h */
