/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __WEECHAT_COMMAND_H
#define __WEECHAT_COMMAND_H 1

#include "../irc/irc.h"

#define MAX_ARGS 8192

typedef struct t_weechat_command t_weechat_command;

struct t_weechat_command
{
    char *command_name;
    char *command_description;
    char *arguments;
    char *arguments_description;
    int min_arg, max_arg;
    int (*cmd_function_args)(int, char **);
    int (*cmd_function_1arg)(char *);
};

typedef struct t_weechat_alias t_weechat_alias;

struct t_weechat_alias
{
    char *alias_name;
    char *alias_command;
    t_weechat_alias *prev_alias;
    t_weechat_alias *next_alias;
};

typedef struct t_index_command t_index_command;

struct t_index_command
{
    char *command_name;
    t_index_command *prev_index;
    t_index_command *next_index;
};

extern t_weechat_alias *weechat_alias;
extern t_index_command *index_commands;

extern t_index_command *index_command_search (char *);
extern t_index_command *index_command_new (char *);
extern void index_command_build ();
extern t_weechat_alias *alias_new (char *, char *);
extern int exec_weechat_command (t_irc_server *, char *);
extern void user_command (t_irc_server *, char *);
extern int weechat_cmd_alias (char *);
extern int weechat_cmd_clear (int, char **);
extern int weechat_cmd_connect (int, char **);
extern int weechat_cmd_disconnect (int, char **);
extern int weechat_cmd_help (int, char **);
extern int weechat_cmd_perl (int, char **);
extern int weechat_cmd_save (int, char **);
extern int weechat_cmd_server (int, char **);
extern int weechat_cmd_set (int, char **);
extern int weechat_cmd_unalias (char *);

#endif /* command.h */
