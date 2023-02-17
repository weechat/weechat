/*
 * Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_EXEC_COMMAND_H
#define WEECHAT_PLUGIN_EXEC_COMMAND_H

struct t_exec_cmd_options
{
    int command_index;                 /* index of command in arguments     */
    int use_shell;                     /* 1 to use shell (sh -c "command")  */
    int detached;                      /* 1 if detached (no output)         */
    int pipe_stdin;                    /* 1 to create a pipe for stdin      */
    int timeout;                       /* timeout (in seconds)              */
    const char *ptr_buffer_name;       /* name of buffer                    */
    struct t_gui_buffer *ptr_buffer;   /* pointer to buffer                 */
    int output_to_buffer;              /* 1 if output is sent to buffer     */
    int output_to_buffer_exec_cmd;     /* execute commands found            */
    int output_to_buffer_stderr;       /* 1 if stderr is sent to buffer     */
    int new_buffer;                    /* 1=new buffer, 2=new buf. free cont*/
    int new_buffer_clear;              /* 1 to clear buffer before output   */
    int switch_to_buffer;              /* switch to the output buffer       */
    int line_numbers;                  /* 1 to display line numbers         */
    int flush;                         /* 1 to flush lines immediately      */
    int color;                         /* what to do with ANSI colors       */
    int display_rc;                    /* 1 to display return code          */
    const char *ptr_command_name;      /* name of command                   */
    char *pipe_command;                /* output piped to WeeChat/plugin cmd*/
    char *hsignal;                     /* send a hsignal with output        */
};

extern int exec_command_run (struct t_gui_buffer *buffer,
                             int argc, char **argv, char **argv_eol,
                             int start_arg);
extern void exec_command_init ();

#endif /* WEECHAT_PLUGIN_EXEC_COMMAND_H */
