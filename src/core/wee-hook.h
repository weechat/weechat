/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_HOOK_H
#define WEECHAT_HOOK_H

struct t_hook;

#include "hook/wee-hook-command-run.h"
#include "hook/wee-hook-command.h"
#include "hook/wee-hook-completion.h"
#include "hook/wee-hook-config.h"
#include "hook/wee-hook-connect.h"
#include "hook/wee-hook-fd.h"
#include "hook/wee-hook-focus.h"
#include "hook/wee-hook-hdata.h"
#include "hook/wee-hook-hsignal.h"
#include "hook/wee-hook-info-hashtable.h"
#include "hook/wee-hook-info.h"
#include "hook/wee-hook-infolist.h"
#include "hook/wee-hook-line.h"
#include "hook/wee-hook-modifier.h"
#include "hook/wee-hook-print.h"
#include "hook/wee-hook-process.h"
#include "hook/wee-hook-signal.h"
#include "hook/wee-hook-timer.h"

struct t_gui_bar;
struct t_gui_buffer;
struct t_gui_line;
struct t_gui_completion;
struct t_gui_window;
struct t_weelist;
struct t_hashtable;
struct t_infolist;
struct t_infolist_item;

/* hook types */

enum t_hook_type
{
    HOOK_TYPE_COMMAND = 0,             /* new command                       */
    HOOK_TYPE_COMMAND_RUN,             /* when a command is executed        */
    HOOK_TYPE_TIMER,                   /* timer                             */
    HOOK_TYPE_FD,                      /* socket of file descriptor         */
    HOOK_TYPE_PROCESS,                 /* sub-process (fork)                */
    HOOK_TYPE_CONNECT,                 /* connect to peer with fork         */
    HOOK_TYPE_LINE,                    /* new line in a buffer              */
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

typedef void (t_callback_hook)(struct t_hook *hook);
typedef char *(t_callback_hook_get_desc)(struct t_hook *hook);
typedef int (t_callback_hook_infolist)(struct t_infolist_item *item,
                                       struct t_hook *hook);

struct t_hook
{
    /* data common to all hooks */
    struct t_weechat_plugin *plugin;   /* plugin which created this hook    */
                                       /* (NULL for hook created by WeeChat)*/
    char *subplugin;                   /* subplugin which created this hook */
                                       /* (commonly a script name, NULL for */
                                       /* hook created by WeeChat or by     */
                                       /* plugin itself)                    */
    enum t_hook_type type;             /* hook type                         */
    int deleted;                       /* hook marked for deletion ?        */
    int running;                       /* 1 if hook is currently running    */
    int priority;                      /* priority (to sort hooks)          */
    const void *callback_pointer;      /* pointer sent to callback          */
    void *callback_data;               /* data sent to callback             */

    /* hook data (depends on hook type) */
    void *hook_data;                   /* hook specific data                */
    struct t_hook *prev_hook;          /* link to previous hook             */
    struct t_hook *next_hook;          /* link to next hook                 */
};

/* hook variables */

extern char *hook_type_string[];
extern struct t_hook *weechat_hooks[];
extern struct t_hook *last_weechat_hook[];
extern int hooks_count[];
extern int hooks_count_total;
extern int hook_socketpair_ok;

/* hook functions */

extern void hook_init ();
extern void hook_add_to_list (struct t_hook *new_hook);
extern void hook_init_data (struct t_hook *hook,
                            struct t_weechat_plugin *plugin,
                            int type, int priority,
                            const void *callback_pointer, void *callback_data);
extern int hook_valid (struct t_hook *hook);
extern void hook_exec_start ();
extern void hook_exec_end ();
extern char *hook_get_description (struct t_hook *hook);
extern void hook_set (struct t_hook *hook, const char *property,
                      const char *value);
extern void unhook (struct t_hook *hook);
extern void unhook_all_plugin (struct t_weechat_plugin *plugin,
                               const char *subplugin);
extern void unhook_all ();
extern int hook_add_to_infolist (struct t_infolist *infolist,
                                 struct t_hook *hook,
                                 const char *arguments);
extern void hook_print_log ();

#endif /* WEECHAT_HOOK_H */
