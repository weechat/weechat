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


#ifndef __WEECHAT_HOOK_H
#define __WEECHAT_HOOK_H 1

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

struct t_gui_buffer;
struct t_weelist;

/* hook types */

enum t_hook_type
{
    HOOK_TYPE_COMMAND = 0,             /* new command                       */
    HOOK_TYPE_TIMER,                   /* timer                             */
    HOOK_TYPE_FD,                      /* socket of file descriptor         */
    HOOK_TYPE_CONNECT,                 /* connect to peer with fork         */
    HOOK_TYPE_PRINT,                   /* printed message                   */
    HOOK_TYPE_SIGNAL,                  /* signal                            */
    HOOK_TYPE_CONFIG,                  /* config option                     */
    HOOK_TYPE_COMPLETION,              /* custom completions                */
    HOOK_TYPE_MODIFIER,                /* string modifier                   */
    /* number of hook types */
    HOOK_NUM_TYPES,
};

/* max calls that can be done for a command (recursive calls) */
#define HOOK_COMMAND_MAX_CALLS  5

/* flags for fd hooks */
#define HOOK_FD_FLAG_READ       1
#define HOOK_FD_FLAG_WRITE      2
#define HOOK_FD_FLAG_EXCEPTION  4

/* macros to access hook specific data */
#define HOOK_COMMAND(hook, var) (((struct t_hook_command *)hook->hook_data)->var)
#define HOOK_TIMER(hook, var) (((struct t_hook_timer *)hook->hook_data)->var)
#define HOOK_FD(hook, var) (((struct t_hook_fd *)hook->hook_data)->var)
#define HOOK_CONNECT(hook, var) (((struct t_hook_connect *)hook->hook_data)->var)
#define HOOK_PRINT(hook, var) (((struct t_hook_print *)hook->hook_data)->var)
#define HOOK_SIGNAL(hook, var) (((struct t_hook_signal *)hook->hook_data)->var)
#define HOOK_CONFIG(hook, var) (((struct t_hook_config *)hook->hook_data)->var)
#define HOOK_COMPLETION(hook, var) (((struct t_hook_completion *)hook->hook_data)->var)
#define HOOK_MODIFIER(hook, var) (((struct t_hook_modifier *)hook->hook_data)->var)

struct t_hook
{
    /* data common to all hooks */
    struct t_weechat_plugin *plugin;   /* plugin which created this hook    */
                                       /* (NULL for hook created by WeeChat)*/
    enum t_hook_type type;             /* hook type                         */
    int deleted;                       /* hook marked for deletion ?        */
    int running;                       /* 1 if hook is currently running    */
    void *callback_data;               /* data sent to callback             */
    
    /* hook data (depends on hook type) */
    void *hook_data;                   /* hook specific data                */
    struct t_hook *prev_hook;          /* link to previous hook             */
    struct t_hook *next_hook;          /* link to next hook                 */
};

typedef int (t_hook_callback_command)(void *data, struct t_gui_buffer *buffer,
                                      int argc, char **argv, char **argv_eol);

struct t_hook_command
{
    t_hook_callback_command *callback; /* command callback                  */
    char *command;                     /* name of command (without '/')     */
    int level;                         /* when many commands with same name */
                                       /* exist: lower level= high priority */
    char *description;                 /* (for /help) short cmd description */
    char *args;                        /* (for /help) command arguments     */
    char *args_description;            /* (for /help) args long description */
    char *completion;                  /* template for completion           */
};

typedef int (t_hook_callback_timer)(void *data);

struct t_hook_timer
{
    t_hook_callback_timer *callback;   /* timer callback                    */
    long interval;                     /* timer interval (milliseconds)     */
    int remaining_calls;               /* calls remaining (0 = unlimited)   */
    struct timeval last_exec;          /* last time hook was executed       */
    struct timeval next_exec;          /* next scheduled execution          */
};

typedef int (t_hook_callback_fd)(void *data);

struct t_hook_fd
{
    t_hook_callback_fd *callback;      /* fd callback                       */
    int fd;                            /* socket or file descriptor         */
    int flags;                         /* fd flags (read,write,..)          */
};

typedef int (t_hook_callback_connect)(void *data, int status);

struct t_hook_connect
{
    t_hook_callback_connect *callback; /* connect callback                  */
    char *address;                     /* peer address                      */
    int port;                          /* peer port                         */
    int sock;                          /* socket (created by caller)        */
    int ipv6;                          /* IPv6 connection ?                 */
#ifdef HAVE_GNUTLS
    gnutls_session_t *gnutls_sess;     /* GnuTLS session (SSL connection)   */
#endif
    char *local_hostname;              /* force local hostname (optional)   */
    int child_read;                    /* to read into child pipe           */
    int child_write;                   /* to write into child pipe          */
    pid_t child_pid;                   /* pid of child process (connecting) */
    struct t_hook *hook_fd;            /* pointer to fd hook                */
};

typedef int (t_hook_callback_print)(void *data, struct t_gui_buffer *buffer,
                                    time_t date, int tags_count,
                                    char **tags, char *prefix,
                                    char *message);

struct t_hook_print
{
    t_hook_callback_print *callback;   /* print callback                    */
    struct t_gui_buffer *buffer;       /* buffer selected (NULL = all)      */
    int tags_count;                    /* number of tags selected           */
    char **tags_array;                 /* tags selected (NULL = any)        */
    char *message;                     /* part of message (NULL/empty = all)*/
    int strip_colors;                  /* strip colors in msg for callback? */
};

typedef int (t_hook_callback_signal)(void *data, char *signal,
                                     char *type_data, void *signal_data);

struct t_hook_signal
{
    t_hook_callback_signal *callback;  /* signal callback                   */
    char *signal;                      /* signal selected (may begin or end */
                                       /* with "*", "*" == any signal)      */
};

typedef int (t_hook_callback_config)(void *data, char *option, char *value);

struct t_hook_config
{
    t_hook_callback_config *callback;  /* config callback                   */
    char *option;                      /* config option for hook            */
                                       /* (NULL = hook for all options)     */
};

typedef int (t_hook_callback_completion)(void *data, char *completion,
                                         struct t_gui_buffer *buffer,
                                         struct t_weelist *list);

struct t_hook_completion
{
    t_hook_callback_completion *callback; /* completion callback            */
    char *completion;                     /* name of completion             */
};

typedef char *(t_hook_callback_modifier)(void *data, char *modifier,
                                         char *modifier_data, char *string);

struct t_hook_modifier
{
    t_hook_callback_modifier *callback; /* modifier callback                */
    char *modifier;                     /* name of modifier                 */
};

/* hook variables */

extern struct t_hook *weechat_hooks[];
extern struct t_hook *last_weechat_hook[];

/* hook functions */

extern void hook_init ();
extern int hook_valid (struct t_hook *hook);
extern int hook_valid_for_plugin (struct t_weechat_plugin *plugin,
                                  struct t_hook *hook);
extern struct t_hook *hook_command (struct t_weechat_plugin *plugin,
                                    char *command, char *description,
                                    char *args, char *args_description,
                                    char *completion,
                                    t_hook_callback_command *callback,
                                    void *callback_data);
extern int hook_command_exec (struct t_gui_buffer *buffer, int any_plugin,
                              struct t_weechat_plugin *plugin, char *string);
extern struct t_hook *hook_timer (struct t_weechat_plugin *plugin,
                                  long interval, int align_second,
                                  int max_calls,
                                  t_hook_callback_timer *callback,
                                  void *callback_data);
extern int hook_timer_time_to_next (struct timeval *tv_timeout);
extern void hook_timer_exec ();
extern struct t_hook *hook_fd (struct t_weechat_plugin *plugin, int fd,
                               int flag_read, int flag_write,
                               int flag_exception,
                               t_hook_callback_fd * callback,
                               void *callback_data);
extern int hook_fd_set (fd_set *read_fds, fd_set *write_fds,
                        fd_set *exception_fds);
extern void hook_fd_exec (fd_set *read_fds, fd_set *write_fds,
                          fd_set *exception_fds);
extern struct t_hook *hook_connect (struct t_weechat_plugin *plugin,
                                    char *address, int port,
                                    int sock, int ipv6, void *gnutls_session,
                                    char *local_hostname,
                                    t_hook_callback_connect * callback,
                                    void *callback_data);
extern struct t_hook *hook_print (struct t_weechat_plugin *plugin,
                                  struct t_gui_buffer *buffer,
                                  char *tags, char *message,
                                  int strip_colors,
                                  t_hook_callback_print *callback,
                                  void *callback_data);
extern void hook_print_exec (struct t_gui_buffer *buffer,
                             time_t date, int tags_count,
                             char **tags_array, char *prefix,
                             char *message);
extern struct t_hook *hook_signal (struct t_weechat_plugin *plugin,
                                   char *signal,
                                   t_hook_callback_signal *callback,
                                   void *callback_data);
extern void hook_signal_send (char *signal, char *type_data,
                              void *signal_data);
extern struct t_hook *hook_config (struct t_weechat_plugin *, char *option,
                                   t_hook_callback_config *callback,
                                   void *callback_data);
extern void hook_config_exec (char *option, char *value);
extern struct t_hook *hook_completion (struct t_weechat_plugin *plugin,
                                       char *completion,
                                       t_hook_callback_completion *callback,
                                       void *callback_data);
extern void hook_completion_exec (struct t_weechat_plugin *plugin,
                                  char *completion,
                                  struct t_gui_buffer *buffer,
                                  struct t_weelist *list);
extern struct t_hook *hook_modifier (struct t_weechat_plugin *plugin,
                                     char *modifier,
                                     t_hook_callback_modifier *callback,
                                     void *callback_data);
extern char *hook_modifier_exec (struct t_weechat_plugin *plugin,
                                 char *modifier, char *modifier_data,
                                 char *string);
extern void unhook (struct t_hook *hook);
extern void unhook_all_plugin (struct t_weechat_plugin *plugin);
extern void unhook_all ();
extern void hook_print_log ();

#endif /* wee-hook.h */
