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

#ifndef __WEECHAT_EXEC_COMMAND_H
#define __WEECHAT_EXEC_COMMAND_H 1

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
    int new_buffer;                    /* output in a new buffer            */
    int switch_to_buffer;              /* switch to the output buffer       */
    int line_numbers;                  /* 1 to display line numbers         */
    const char *ptr_command_name;      /* name of command                   */
};

extern int exec_command_run (struct t_gui_buffer *buffer,
                             int argc, char **argv, char **argv_eol,
                             int start_arg);
extern void exec_command_init ();

#endif /* __WEECHAT_EXEC_COMMAND_H */
