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
    int command_index;
    int use_shell;
    int detached;
    int pipe_stdin;
    int timeout;
    int output_to_buffer;
    const char *ptr_name;
};

extern void exec_command_init ();

#endif /* __WEECHAT_EXEC_COMMAND_H */
