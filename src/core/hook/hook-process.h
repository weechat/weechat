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

#ifndef WEECHAT_HOOK_PROCESS_H
#define WEECHAT_HOOK_PROCESS_H

struct t_weechat_plugin;
struct t_infolist_item;
struct t_hashtable;

#define HOOK_PROCESS(hook, var) (((struct t_hook_process *)hook->hook_data)->var)

/* constants for hook process */
#define HOOK_PROCESS_STDIN       0
#define HOOK_PROCESS_STDOUT      1
#define HOOK_PROCESS_STDERR      2
#define HOOK_PROCESS_BUFFER_SIZE 65536

typedef int (t_hook_callback_process)(const void *pointer, void *data,
                                      const char *command,
                                      int return_code,
                                      const char *out, const char *err);

struct t_hook_process
{
    t_hook_callback_process *callback; /* process callback (after child end)*/
    char *command;                     /* command executed by child         */
    struct t_hashtable *options;       /* options for process (see doc)     */
    int detached;                      /* detached mode (background)        */
    long timeout;                      /* timeout (ms) (0 = no timeout)     */
    int child_read[3];                 /* read stdin/out/err data from child*/
    int child_write[3];                /* write stdin/out/err data for child*/
    pid_t child_pid;                   /* pid of child process              */
    struct t_hook *hook_fd[3];         /* hook fd for stdin/out/err         */
    struct t_hook *hook_timer;         /* timer to check if child has died  */
    char *buffer[3];                   /* buffers for child stdin/out/err   */
    int buffer_size[3];                /* size of child stdin/out/err       */
    int buffer_flush;                  /* bytes to flush output buffers     */
};

extern int hook_process_pending;

extern char *hook_process_get_description (struct t_hook *hook);
extern struct t_hook *hook_process (struct t_weechat_plugin *plugin,
                                    const char *command,
                                    int timeout,
                                    t_hook_callback_process *callback,
                                    const void *callback_pointer,
                                    void *callback_data);
extern struct t_hook *hook_process_hashtable (struct t_weechat_plugin *plugin,
                                              const char *command,
                                              struct t_hashtable *options,
                                              int timeout,
                                              t_hook_callback_process *callback,
                                              const void *callback_pointer,
                                              void *callback_data);
extern void hook_process_exec (void);
extern void hook_process_free_data (struct t_hook *hook);
extern int hook_process_add_to_infolist (struct t_infolist_item *item,
                                         struct t_hook *hook);
extern void hook_process_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_PROCESS_H */
