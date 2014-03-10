/*
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_EXEC_H
#define __WEECHAT_EXEC_H 1

#include <time.h>

#define weechat_plugin weechat_exec_plugin
#define EXEC_PLUGIN_NAME "exec"

struct t_exec_cmd
{
    /* command/process */
    int number;                        /* command number                    */
    char *name;                        /* name of command                   */
    struct t_hook *hook;               /* pointer to process hook           */
    char *command;                     /* command (with arguments)          */
    pid_t pid;                         /* process id                        */
    time_t start_time;                 /* start time                        */
    time_t end_time;                   /* end time                          */

    /* buffer */
    char *buffer_plugin;               /* buffer plugin (where cmd is exec) */
    char *buffer_name;                 /* buffer name (where cmd is exec)   */
    int output_to_buffer;              /* 1 if output is sent to buffer     */

    /* command output */
    int stdout_size;                   /* number of bytes in stdout         */
    char *stdout;                      /* stdout of command                 */
    int stderr_size;                   /* number of bytes in stderr         */
    char *stderr;                      /* stderr of command                 */
    int return_code;                   /* command return code               */

    struct t_exec_cmd *prev_cmd;       /* link to previous command          */
    struct t_exec_cmd *next_cmd;       /* link to next command              */
};

extern struct t_weechat_plugin *weechat_exec_plugin;
extern struct t_exec_cmd *exec_cmds;
extern struct t_exec_cmd *last_exec_cmd;
extern int exec_cmds_count;

extern struct t_exec_cmd *exec_search_by_id (const char *id);
extern struct t_exec_cmd *exec_add ();
extern int exec_process_cb (void *data, const char *command, int return_code,
                            const char *out, const char *err);
extern void exec_free (struct t_exec_cmd *exec_cmd);
extern void exec_free_all ();

#endif /* __WEECHAT_EXEC_H */
