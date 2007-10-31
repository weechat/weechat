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


#ifndef __WEECHAT_COMMAND_H
#define __WEECHAT_COMMAND_H 1

#include "weelist.h"
#include "../protocols/irc/irc.h"
#include "../gui/gui.h"

#define MAX_ARGS 8192

typedef struct t_weechat_command t_weechat_command;

struct t_weechat_command
{
    char *command_name;             /* WeeChat (internal) command name        */
    char *command_description;      /* command description (for /help)        */
    char *arguments;                /* command arguments (for /help)          */
    char *arguments_description;    /* arguments description (for /help)      */
    char *completion_template;      /* template for completion                */
                                    /* NULL=no completion, ""=default (nick)  */
    int min_arg, max_arg;           /* min & max number of arguments          */
    int conversion;                 /* = 1 if cmd args are converted (charset */
                                    /* and color) before execution            */
    int (*cmd_function_args)(t_irc_server *, t_irc_channel *, int, char **);
                                    /* function called when user enters cmd   */
    int (*cmd_function_1arg)(t_irc_server *, t_irc_channel *, char *);
                                    /* function called when user enters cmd   */
};

extern t_weechat_command weechat_commands[];

extern t_weelist *index_commands;
extern t_weelist *last_index_command;

extern void command_index_build ();
extern void command_index_free ();
extern int command_used_by_weechat (char *);
extern char **split_multi_command (char *, char);
extern void free_multi_command (char **);
extern int exec_weechat_command (t_irc_server *, t_irc_channel *, char *, int);
extern void user_command (t_irc_server *, t_irc_channel *, char *, int);
extern int weechat_cmd_alias (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_buffer (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_builtin (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_clear (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_connect (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_dcc (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_debug (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_disconnect (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_help (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_history (t_irc_server *, t_irc_channel *, int, char **);
extern void weechat_cmd_ignore_display (char *, t_irc_ignore *);
extern int weechat_cmd_ignore (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_key (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_panel (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_plugin (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_reconnect (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_save (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_server (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_set (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_setp (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_unalias (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_unignore (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_upgrade (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_uptime (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_window (t_irc_server *, t_irc_channel *, int, char **);

#endif /* command.h */
