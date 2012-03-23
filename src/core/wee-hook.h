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

#ifndef __WEECHAT_HOOK_H
#define __WEECHAT_HOOK_H 1

#include <unistd.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

struct t_gui_bar;
struct t_gui_buffer;
struct t_gui_line;
struct t_gui_completion;
struct t_gui_window;
struct t_weelist;
struct t_hashtable;
struct t_infolist;

/* hook types */

enum t_hook_type
{
    HOOK_TYPE_COMMAND = 0,             /* new command                       */
    HOOK_TYPE_COMMAND_RUN,             /* when a command is executed        */
    HOOK_TYPE_TIMER,                   /* timer                             */
    HOOK_TYPE_FD,                      /* socket of file descriptor         */
    HOOK_TYPE_PROCESS,                 /* sub-process (fork)                */
    HOOK_TYPE_CONNECT,                 /* connect to peer with fork         */
    HOOK_TYPE_PRINT,                   /* printed message                   */
    HOOK_TYPE_SIGNAL,                  /* signal                            */
    HOOK_TYPE_HSIGNAL,                 /* signal (using hashtable)          */
    HOOK_TYPE_CONFIG,                  /* config option                     */
    HOOK_TYPE_COMPLETION,              /* custom completions                */
    HOOK_TYPE_MODIFIER,                /* string modifier                   */
    HOOK_TYPE_INFO,                    /* get some info as string           */
    HOOK_TYPE_INFO_HASHTABLE,          /* get some info as hashtable        */
    HOOK_TYPE_INFOLIST,                /* get some info as infolist         */
    HOOK_TYPE_HDATA,                   /* get hdata pointer                 */
    HOOK_TYPE_FOCUS,                   /* focus event (mouse/key)           */
    /* number of hook types */
    HOOK_NUM_TYPES,
};

/*
 * default priority: higher value means higher priority, ie added at the
 * beginning of the hook list
 */
#define HOOK_PRIORITY_DEFAULT   1000

/* max calls that can be done for a command (recursive calls) */
#define HOOK_COMMAND_MAX_CALLS  5

/* flags for fd hooks */
#define HOOK_FD_FLAG_READ       1
#define HOOK_FD_FLAG_WRITE      2
#define HOOK_FD_FLAG_EXCEPTION  4

/* constants for hook process */
#define HOOK_PROCESS_STDOUT      0
#define HOOK_PROCESS_STDERR      1
#define HOOK_PROCESS_BUFFER_SIZE 65536

/* macros to access hook specific data */
#define HOOK_COMMAND(hook, var) (((struct t_hook_command *)hook->hook_data)->var)
#define HOOK_COMMAND_RUN(hook, var) (((struct t_hook_command_run *)hook->hook_data)->var)
#define HOOK_TIMER(hook, var) (((struct t_hook_timer *)hook->hook_data)->var)
#define HOOK_FD(hook, var) (((struct t_hook_fd *)hook->hook_data)->var)
#define HOOK_PROCESS(hook, var) (((struct t_hook_process *)hook->hook_data)->var)
#define HOOK_CONNECT(hook, var) (((struct t_hook_connect *)hook->hook_data)->var)
#define HOOK_PRINT(hook, var) (((struct t_hook_print *)hook->hook_data)->var)
#define HOOK_SIGNAL(hook, var) (((struct t_hook_signal *)hook->hook_data)->var)
#define HOOK_HSIGNAL(hook, var) (((struct t_hook_hsignal *)hook->hook_data)->var)
#define HOOK_CONFIG(hook, var) (((struct t_hook_config *)hook->hook_data)->var)
#define HOOK_COMPLETION(hook, var) (((struct t_hook_completion *)hook->hook_data)->var)
#define HOOK_MODIFIER(hook, var) (((struct t_hook_modifier *)hook->hook_data)->var)
#define HOOK_INFO(hook, var) (((struct t_hook_info *)hook->hook_data)->var)
#define HOOK_INFO_HASHTABLE(hook, var) (((struct t_hook_info_hashtable *)hook->hook_data)->var)
#define HOOK_INFOLIST(hook, var) (((struct t_hook_infolist *)hook->hook_data)->var)
#define HOOK_HDATA(hook, var) (((struct t_hook_hdata *)hook->hook_data)->var)
#define HOOK_FOCUS(hook, var) (((struct t_hook_focus *)hook->hook_data)->var)

struct t_hook
{
    /* data common to all hooks */
    struct t_weechat_plugin *plugin;   /* plugin which created this hook    */
                                       /* (NULL for hook created by WeeChat)*/
    enum t_hook_type type;             /* hook type                         */
    int deleted;                       /* hook marked for deletion ?        */
    int running;                       /* 1 if hook is currently running    */
    int priority;                      /* priority (to sort hooks)          */
    void *callback_data;               /* data sent to callback             */

    /* hook data (depends on hook type) */
    void *hook_data;                   /* hook specific data                */
    struct t_hook *prev_hook;          /* link to previous hook             */
    struct t_hook *next_hook;          /* link to next hook                 */
};

/* hook command */

typedef int (t_hook_callback_command)(void *data, struct t_gui_buffer *buffer,
                                      int argc, char **argv, char **argv_eol);

struct t_hook_command
{
    t_hook_callback_command *callback;  /* command callback                 */
    char *command;                      /* name of command (without '/')    */
    char *description;                  /* (for /help) short cmd description*/
    char *args;                         /* (for /help) command arguments    */
    char *args_description;             /* (for /help) args long description*/
    char *completion;                   /* template for completion          */

    /* templates */
    int cplt_num_templates;             /* number of templates for compl.   */
    char **cplt_templates;              /* completion templates             */
    char **cplt_templates_static;       /* static part of template (at      */
                                        /* beginning                        */

    /* arguments for each template */
    int *cplt_template_num_args;        /* number of arguments for template */
    char ***cplt_template_args;         /* arguments for each template      */

    /* concatenation of arg N for each template */
    int cplt_template_num_args_concat; /* number of concatened arguments    */
    char **cplt_template_args_concat;  /* concatened arguments              */
};

/* hook command run */

typedef int (t_hook_callback_command_run)(void *data,
                                          struct t_gui_buffer *buffer,
                                          const char *command);

struct t_hook_command_run
{
    t_hook_callback_command_run *callback; /* command_run callback          */
    char *command;                     /* name of command (without '/')     */
};

/* hook timer */

typedef int (t_hook_callback_timer)(void *data, int remaining_calls);

struct t_hook_timer
{
    t_hook_callback_timer *callback;   /* timer callback                    */
    long interval;                     /* timer interval (milliseconds)     */
    int align_second;                  /* alignment on a second             */
                                       /* for ex.: 60 = each min. at 0 sec  */
    int remaining_calls;               /* calls remaining (0 = unlimited)   */
    struct timeval last_exec;          /* last time hook was executed       */
    struct timeval next_exec;          /* next scheduled execution          */
};

/* hook fd */

typedef int (t_hook_callback_fd)(void *data, int fd);

struct t_hook_fd
{
    t_hook_callback_fd *callback;      /* fd callback                       */
    int fd;                            /* socket or file descriptor         */
    int flags;                         /* fd flags (read,write,..)          */
    int error;                         /* contains errno if error occurred  */
                                       /* with fd                           */
};

/* hook process */

typedef int (t_hook_callback_process)(void *data, const char *command,
                                      int return_code, const char *out,
                                      const char *err);

struct t_hook_process
{
    t_hook_callback_process *callback; /* process callback (after child end)*/
    char *command;                     /* command executed by child         */
    struct t_hashtable *options;       /* options for process (see doc)     */
    long timeout;                      /* timeout (ms) (0 = no timeout)     */
    int child_read[2];                 /* to read data in pipe from child   */
    int child_write[2];                /* to write data in pipe for child   */
    pid_t child_pid;                   /* pid of child process              */
    struct t_hook *hook_fd[2];         /* hook fd for stdout/stderr         */
    struct t_hook *hook_timer;         /* timer to check if child has died  */
    char *buffer[2];                   /* buffers for child stdout/stderr   */
    int buffer_size[2];                /* size of child stdout/stderr       */
};

/* hook connect */

typedef int (t_hook_callback_connect)(void *data, int status,
                                      int gnutls_rc,
                                      const char *error,
                                      const char *ip_address);

#ifdef HAVE_GNUTLS
typedef int (gnutls_callback_t)(void *data, gnutls_session_t tls_session,
                                const gnutls_datum_t *req_ca, int nreq,
                                const gnutls_pk_algorithm_t *pk_algos,
                                int pk_algos_len,
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00
                                gnutls_retr2_st *answer,
#else
                                gnutls_retr_st *answer,
#endif
                                int action);
#endif

struct t_hook_connect
{
    t_hook_callback_connect *callback; /* connect callback                  */
    char *proxy;                       /* proxy (optional)                  */
    char *address;                     /* peer address                      */
    int port;                          /* peer port                         */
    int sock;                          /* socket (created by caller)        */
    int ipv6;                          /* IPv6 connection ?                 */
#ifdef HAVE_GNUTLS
    gnutls_session_t *gnutls_sess;     /* GnuTLS session (SSL connection)   */
    gnutls_callback_t *gnutls_cb;      /* GnuTLS callback during handshake  */
    int gnutls_dhkey_size;             /* Diffie Hellman Key Exchange size  */
    char *gnutls_priorities;           /* GnuTLS priorities                 */
#endif
    char *local_hostname;              /* force local hostname (optional)   */
    int child_read;                    /* to read data in pipe from child   */
    int child_write;                   /* to write data in pipe for child   */
    pid_t child_pid;                   /* pid of child process (connecting) */
    struct t_hook *hook_child_timer;   /* timer for child process timeout   */
    struct t_hook *hook_fd;            /* pointer to fd hook                */
    struct t_hook *handshake_hook_fd;  /* fd hook for handshake             */
    struct t_hook *handshake_hook_timer; /* timer for handshake timeout     */
    int handshake_fd_flags;            /* socket flags saved for handshake  */
    char *handshake_ip_address;        /* ip address (used for handshake)   */
};

/* hook print */

typedef int (t_hook_callback_print)(void *data, struct t_gui_buffer *buffer,
                                    time_t date, int tags_count,
                                    const char **tags, int displayed,
                                    int highlight, const char *prefix,
                                    const char *message);

struct t_hook_print
{
    t_hook_callback_print *callback;   /* print callback                    */
    struct t_gui_buffer *buffer;       /* buffer selected (NULL = all)      */
    int tags_count;                    /* number of tags selected           */
    char **tags_array;                 /* tags selected (NULL = any)        */
    char *message;                     /* part of message (NULL/empty = all)*/
    int strip_colors;                  /* strip colors in msg for callback? */
};

/* hook signal */

typedef int (t_hook_callback_signal)(void *data, const char *signal,
                                     const char *type_data, void *signal_data);

struct t_hook_signal
{
    t_hook_callback_signal *callback;  /* signal callback                   */
    char *signal;                      /* signal selected (may begin or end */
                                       /* with "*", "*" == any signal)      */
};

/* hook hsignal */

typedef int (t_hook_callback_hsignal)(void *data, const char *signal,
                                      struct t_hashtable *hashtable);

struct t_hook_hsignal
{
    t_hook_callback_hsignal *callback; /* signal callback                   */
    char *signal;                      /* signal selected (may begin or end */
                                       /* with "*", "*" == any signal)      */
};

/* hook config */

typedef int (t_hook_callback_config)(void *data, const char *option,
                                     const char *value);

struct t_hook_config
{
    t_hook_callback_config *callback;  /* config callback                   */
    char *option;                      /* config option for hook            */
                                       /* (NULL = hook for all options)     */
};

/* hook completion */

typedef int (t_hook_callback_completion)(void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion);

struct t_hook_completion
{
    t_hook_callback_completion *callback; /* completion callback            */
    char *completion_item;                /* name of completion             */
    char *description;                    /* description                    */
};

/* hook modifier */

typedef char *(t_hook_callback_modifier)(void *data, const char *modifier,
                                         const char *modifier_data,
                                         const char *string);

struct t_hook_modifier
{
    t_hook_callback_modifier *callback; /* modifier callback                */
    char *modifier;                     /* name of modifier                 */
};

/* hook info */

typedef const char *(t_hook_callback_info)(void *data, const char *info_name,
                                           const char *arguments);

struct t_hook_info
{
    t_hook_callback_info *callback;    /* info callback                     */
    char *info_name;                   /* name of info returned             */
    char *description;                 /* description                       */
    char *args_description;            /* description of arguments          */
};

/* hook info (hashtable) */

typedef struct t_hashtable *(t_hook_callback_info_hashtable)(void *data,
                                                             const char *info_name,
                                                             struct t_hashtable *hashtable);

struct t_hook_info_hashtable
{
    t_hook_callback_info_hashtable *callback; /* info_hashtable callback    */
    char *info_name;                   /* name of info returned             */
    char *description;                 /* description                       */
    char *args_description;            /* description of arguments          */
    char *output_description;          /* description of output (hashtable) */
};

/* hook infolist */

typedef struct t_infolist *(t_hook_callback_infolist)(void *data,
                                                      const char *infolist_name,
                                                      void *pointer,
                                                      const char *arguments);

struct t_hook_infolist
{
    t_hook_callback_infolist *callback; /* infolist callback                */
    char *infolist_name;                /* name of infolist returned        */
    char *description;                  /* description                      */
    char *pointer_description;          /* description of pointer           */
    char *args_description;             /* description of arguments         */
};

/* hook hdata */

typedef struct t_hdata *(t_hook_callback_hdata)(void *data,
                                                const char *hdata_name);

struct t_hook_hdata
{
    t_hook_callback_hdata *callback;    /* hdata callback                   */
    char *hdata_name;                   /* hdata name                       */
    char *description;                  /* description                      */
};

/* hook focus */

typedef struct t_hashtable *(t_hook_callback_focus)(void *data,
                                                    struct t_hashtable *info);

struct t_hook_focus
{
    t_hook_callback_focus *callback;    /* focus callback                   */
    char *area;                         /* "chat" or bar item name          */
};

/* hook variables */

extern char *hook_type_string[];
extern struct t_hook *weechat_hooks[];
extern struct t_hook *last_weechat_hook[];

/* hook functions */

extern void hook_init ();
extern struct t_hook *hook_command (struct t_weechat_plugin *plugin,
                                    const char *command,
                                    const char *description,
                                    const char *args,
                                    const char *args_description,
                                    const char *completion,
                                    t_hook_callback_command *callback,
                                    void *callback_data);
extern int hook_command_exec (struct t_gui_buffer *buffer, int any_plugin,
                              struct t_weechat_plugin *plugin,
                              const char *string);
extern struct t_hook *hook_command_run (struct t_weechat_plugin *plugin,
                                        const char *command,
                                        t_hook_callback_command_run *callback,
                                        void *callback_data);
extern int hook_command_run_exec (struct t_gui_buffer *buffer,
                                  const char *command);
extern struct t_hook *hook_timer (struct t_weechat_plugin *plugin,
                                  long interval, int align_second,
                                  int max_calls,
                                  t_hook_callback_timer *callback,
                                  void *callback_data);
extern void hook_timer_time_to_next (struct timeval *tv_timeout);
extern void hook_timer_exec ();
extern struct t_hook *hook_fd (struct t_weechat_plugin *plugin, int fd,
                               int flag_read, int flag_write,
                               int flag_exception,
                               t_hook_callback_fd *callback,
                               void *callback_data);
extern int hook_fd_set (fd_set *read_fds, fd_set *write_fds,
                        fd_set *exception_fds);
extern void hook_fd_exec (fd_set *read_fds, fd_set *write_fds,
                          fd_set *exception_fds);
extern struct t_hook *hook_process (struct t_weechat_plugin *plugin,
                                    const char *command,
                                    int timeout,
                                    t_hook_callback_process *callback,
                                    void *callback_data);
extern struct t_hook *hook_process_hashtable (struct t_weechat_plugin *plugin,
                                              const char *command,
                                              struct t_hashtable *options,
                                              int timeout,
                                              t_hook_callback_process *callback,
                                              void *callback_data);
extern struct t_hook *hook_connect (struct t_weechat_plugin *plugin,
                                    const char *proxy, const char *address,
                                    int port, int sock, int ipv6,
                                    void *gnutls_session, void *gnutls_cb,
                                    int gnutls_dhkey_size,
                                    const char *gnutls_priorities,
                                    const char *local_hostname,
                                    t_hook_callback_connect *callback,
                                    void *callback_data);
#ifdef HAVE_GNUTLS
extern int hook_connect_gnutls_verify_certificates (gnutls_session_t tls_session);
extern int hook_connect_gnutls_set_certificates (gnutls_session_t tls_session,
                                                 const gnutls_datum_t *req_ca, int nreq,
                                                 const gnutls_pk_algorithm_t *pk_algos,
                                                 int pk_algos_len,
#if LIBGNUTLS_VERSION_NUMBER >= 0x020b00
                                                 gnutls_retr2_st *answer);
#else
                                                 gnutls_retr_st *answer);
#endif
#endif
extern struct t_hook *hook_print (struct t_weechat_plugin *plugin,
                                  struct t_gui_buffer *buffer,
                                  const char *tags, const char *message,
                                  int strip_colors,
                                  t_hook_callback_print *callback,
                                  void *callback_data);
extern void hook_print_exec (struct t_gui_buffer *buffer,
                             struct t_gui_line *line);
extern struct t_hook *hook_signal (struct t_weechat_plugin *plugin,
                                   const char *signal,
                                   t_hook_callback_signal *callback,
                                   void *callback_data);
extern void hook_signal_send (const char *signal, const char *type_data,
                              void *signal_data);
extern struct t_hook *hook_hsignal (struct t_weechat_plugin *plugin,
                                    const char *signal,
                                    t_hook_callback_hsignal *callback,
                                    void *callback_data);
extern void hook_hsignal_send (const char *signal,
                               struct t_hashtable *hashtable);
extern struct t_hook *hook_config (struct t_weechat_plugin *plugin,
                                   const char *option,
                                   t_hook_callback_config *callback,
                                   void *callback_data);
extern void hook_config_exec (const char *option, const char *value);
extern struct t_hook *hook_completion (struct t_weechat_plugin *plugin,
                                       const char *completion_item,
                                       const char *description,
                                       t_hook_callback_completion *callback,
                                       void *callback_data);
extern const char *hook_completion_get_string (struct t_gui_completion *completion,
                                               const char *property);
extern void hook_completion_list_add (struct t_gui_completion *completion,
                                      const char *word, int nick_completion,
                                      const char *where);
extern void hook_completion_exec (struct t_weechat_plugin *plugin,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion);
extern struct t_hook *hook_modifier (struct t_weechat_plugin *plugin,
                                     const char *modifier,
                                     t_hook_callback_modifier *callback,
                                     void *callback_data);
extern char *hook_modifier_exec (struct t_weechat_plugin *plugin,
                                 const char *modifier,
                                 const char *modifier_data,
                                 const char *string);
extern struct t_hook *hook_info (struct t_weechat_plugin *plugin,
                                 const char *info_name,
                                 const char *description,
                                 const char *args_description,
                                 t_hook_callback_info *callback,
                                 void *callback_data);
extern const char *hook_info_get (struct t_weechat_plugin *plugin,
                                  const char *info_name,
                                  const char *arguments);
extern struct t_hook *hook_info_hashtable (struct t_weechat_plugin *plugin,
                                           const char *info_name,
                                           const char *description,
                                           const char *args_description,
                                           const char *output_description,
                                           t_hook_callback_info_hashtable *callback,
                                           void *callback_data);
extern struct t_hashtable *hook_info_get_hashtable (struct t_weechat_plugin *plugin,
                                                    const char *info_name,
                                                    struct t_hashtable *hashtable);
extern struct t_hook *hook_infolist (struct t_weechat_plugin *plugin,
                                     const char *infolist_name,
                                     const char *description,
                                     const char *pointer_description,
                                     const char *args_description,
                                     t_hook_callback_infolist *callback,
                                     void *callback_data);
extern struct t_infolist *hook_infolist_get (struct t_weechat_plugin *plugin,
                                             const char *infolist_name,
                                             void *pointer,
                                             const char *arguments);
extern struct t_hook *hook_hdata (struct t_weechat_plugin *plugin,
                                  const char *hdata_name,
                                  const char *description,
                                  t_hook_callback_hdata *callback,
                                  void *callback_data);
extern struct t_hdata *hook_hdata_get (struct t_weechat_plugin *plugin,
                                       const char *hdata_name);
extern struct t_hook *hook_focus (struct t_weechat_plugin *plugin,
                                  const char *area,
                                  t_hook_callback_focus *callback,
                                  void *callback_data);
extern struct t_hashtable *hook_focus_get_data (struct t_hashtable *hashtable_focus1,
                                                struct t_hashtable *hashtable_focus2);
extern void unhook (struct t_hook *hook);
extern void unhook_all_plugin (struct t_weechat_plugin *plugin);
extern void unhook_all ();
extern int hook_add_to_infolist (struct t_infolist *infolist,
                                 const char *arguments);
extern void hook_print_log ();

#endif /* __WEECHAT_HOOK_H */
