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


#ifndef __WEECHAT_HOOK_H
#define __WEECHAT_HOOK_H 1

/* hook types */

enum t_hook_type
{
    HOOK_TYPE_COMMAND = 0,             /* new command                       */
    HOOK_TYPE_MESSAGE,                 /* message                           */
    HOOK_TYPE_CONFIG,                  /* config option                     */
    HOOK_TYPE_TIMER,                   /* timer (called each N milliseconds)*/
                                       /* (precision is about 20 msec)      */
    HOOK_TYPE_FD,                      /* socket of file descriptor         */
    HOOK_TYPE_KEYBOARD,                /* keyboard handler                  */
};

#define HOOK_FD_FLAG_READ       1
#define HOOK_FD_FLAG_WRITE      2
#define HOOK_FD_FLAG_EXCEPTION  4

#define HOOK_COMMAND(hook, var) (((struct t_hook_command *)hook->hook_data)->var)
#define HOOK_MESSAGE(hook, var) (((struct t_hook_message *)hook->hook_data)->var)
#define HOOK_CONFIG(hook, var) (((struct t_hook_config *)hook->hook_data)->var)
#define HOOK_TIMER(hook, var) (((struct t_hook_timer *)hook->hook_data)->var)
#define HOOK_FD(hook, var) (((struct t_hook_fd *)hook->hook_data)->var)

typedef int (t_hook_callback) (void *);

struct t_hook
{
    /* data common to all hooks */
    struct t_weechat_plugin *plugin;   /* plugin which created this hook    */
                                       /* (NULL for hook created by WeeChat)*/
    enum t_hook_type type;             /* hook type                         */
    void *callback_data;               /* data sent to callback             */

    /* hook data (depends on hook type) */
    void *hook_data;                   /* hook specific data                */
    
    int running;                       /* 1 if hook is currently running    */
    struct t_hook *prev_hook;          /* pointer to previous hook          */
    struct t_hook *next_hook;          /* pointer to next hook              */
};

typedef int (t_hook_callback_command)(void *, char *);

struct t_hook_command
{
    t_hook_callback_command *callback; /* command callback                  */
    char *command;                     /* name of command (without '/')     */
    char *description;                 /* (for /help) short cmd description */
    char *args;                        /* (for /help) command arguments     */
    char *args_description;            /* (for /help) args long description */
    char *completion;                  /* template for completion           */
};

typedef int (t_hook_callback_message)(void *, char *);

struct t_hook_message
{
    t_hook_callback_message *callback; /* message callback                  */
    char *message;                     /* message for hook                  */
};

typedef int (t_hook_callback_config)(void *, char *, char *, char *);

struct t_hook_config
{
    t_hook_callback_config *callback;  /* message callback                  */
    char *type;                        /* "weechat" or "plugin"             */
    char *option;                      /* config option for hook            */
                                       /* (NULL = hook for all options)     */
};

typedef int (t_hook_callback_timer)(void *);

struct t_hook_timer
{
    t_hook_callback_timer *callback;   /* timer callback                    */
    long interval;                     /* timer interval (milliseconds)     */
    int remaining_calls;               /* calls remaining (0 = unlimited)   */
    struct timeval last_exec;          /* last time hook was executed       */
};

typedef int (t_hook_callback_fd)(void *);

struct t_hook_fd
{
    t_hook_callback_fd *callback;      /* fd callback                       */
    int fd;                            /* socket or file descriptor         */
    int flags;                         /* fd flags (read,write,..)          */
};

/* hook variables */

extern struct t_hook *weechat_hooks;
extern struct t_hook *last_weechat_hook;

/* hook functions */

extern int hook_valid_for_plugin (void *, struct t_hook *);
extern struct t_hook *hook_command (void *, char *, char *, char *, char *,
                                    char *, t_hook_callback_command *, void *);
extern int hook_command_exec (void *, char *, char *);
extern struct t_hook *hook_message (void *, char *, t_hook_callback_message *,
                                    void *);
extern struct t_hook *hook_config (void *, char *, char *,
                                   t_hook_callback_config *, void *);
extern void hook_config_exec (char *, char *, char *);
extern struct t_hook *hook_timer (void *, long, int, t_hook_callback_timer *,
                                  void *);
extern void hook_timer_exec (struct timeval *);
extern struct t_hook *hook_fd (void *, int, int, t_hook_callback_fd *,void *);
extern void hook_fd_set (fd_set *, fd_set *, fd_set *);
extern void hook_fd_exec (fd_set *, fd_set *, fd_set *);
extern void unhook (struct t_hook *);
extern void unhook_all_plugin (void *);
extern void unhook_all ();
extern void hook_print_log ();

#endif /* wee-hook.h */
