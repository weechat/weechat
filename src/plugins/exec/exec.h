/*
 * Copyright (C) 2014-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_EXEC_H
#define WEECHAT_PLUGIN_EXEC_H

#include <unistd.h>
#include <time.h>

#define weechat_plugin weechat_exec_plugin
#define EXEC_PLUGIN_NAME "exec"
#define EXEC_PLUGIN_PRIORITY 14000

#define EXEC_STDOUT 0
#define EXEC_STDERR 1

enum t_exec_color
{
    EXEC_COLOR_ANSI = 0,
    EXEC_COLOR_AUTO,
    EXEC_COLOR_IRC,
    EXEC_COLOR_WEECHAT,
    EXEC_COLOR_STRIP,
    /* number of color actions */
    EXEC_NUM_COLORS,
};

struct t_exec_cmd
{
    /* command/process */
    long number;                       /* command number                    */
    char *name;                        /* name of command                   */
    struct t_hook *hook;               /* pointer to process hook           */
    char *command;                     /* command (with arguments)          */
    pid_t pid;                         /* process id                        */
    int detached;                      /* 1 if command is detached          */
    time_t start_time;                 /* start time                        */
    time_t end_time;                   /* end time                          */

    /* display */
    int output_to_buffer;              /* 1 if output is sent to buffer     */
    int output_to_buffer_exec_cmd;     /* 1 if commands are executed        */
    int output_to_buffer_stderr;       /* 1 if stderr is sent to buffer     */
    char *buffer_full_name;            /* buffer where output is displayed  */
    int line_numbers;                  /* 1 if lines numbers are displayed  */
    int color;                         /* what to do with ANSI colors       */
    int display_rc;                    /* 1 if return code is displayed     */

    /* command output */
    int output_line_nb;                /* line number                       */
    int output_size[2];                /* number of bytes in stdout/stderr  */
    char *output[2];                   /* stdout/stderr of command          */
    int return_code;                   /* command return code               */

    /* pipe/hsignal */
    char *pipe_command;                /* output piped to WeeChat/plugin cmd*/
    char *hsignal;                     /* send a hsignal with output        */

    struct t_exec_cmd *prev_cmd;       /* link to previous command          */
    struct t_exec_cmd *next_cmd;       /* link to next command              */
};

extern struct t_weechat_plugin *weechat_exec_plugin;
extern struct t_exec_cmd *exec_cmds;
extern struct t_exec_cmd *last_exec_cmd;
extern int exec_cmds_count;

extern int exec_search_color (const char *color);
extern struct t_exec_cmd *exec_search_by_id (const char *id);
extern struct t_exec_cmd *exec_add ();
extern int exec_process_cb (const void *pointer, void *data,
                            const char *command, int return_code,
                            const char *out, const char *err);
extern void exec_free (struct t_exec_cmd *exec_cmd);
extern void exec_free_all ();

#endif /* WEECHAT_PLUGIN_EXEC_H */
