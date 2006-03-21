/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_COMMAND_H
#define __WEECHAT_COMMAND_H 1

#include "weelist.h"
#include "../irc/irc.h"
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
    int (*cmd_function_args)(t_irc_server *, t_irc_channel *, int, char **);
                                    /* function called when user enters cmd   */
    int (*cmd_function_1arg)(t_irc_server *, t_irc_channel *, char *);
                                    /* function called when user enters cmd   */
};

typedef struct t_weechat_alias t_weechat_alias;

struct t_weechat_alias
{
    char *alias_name;
    char *alias_command;
    int running;
    t_weechat_alias *prev_alias;
    t_weechat_alias *next_alias;
};

extern t_weechat_command weechat_commands[];

extern t_weechat_alias *weechat_alias;
extern t_weelist *index_commands;
extern t_weelist *last_index_command;

extern void command_index_build ();
extern void command_index_free ();
extern t_weechat_alias *alias_search (char *);
extern t_weechat_alias *alias_new (char *, char *);
extern char *alias_get_final_command (t_weechat_alias *);
extern void alias_free_all ();
extern char **explode_string (char *, char *, int, int *);
extern void free_exploded_string (char **);
extern char **split_multi_command (char *, char);
extern void free_multi_command (char **);
extern int exec_weechat_command (t_irc_server *, t_irc_channel *, char *, int);
extern void user_command (t_irc_server *, t_irc_channel *, char *, int);
extern int weechat_cmd_alias (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_buffer (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_builtin (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_charset (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_clear (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_connect (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_debug (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_disconnect (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_help (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_history (t_irc_server *, t_irc_channel *, int, char **);
extern void weechat_cmd_ignore_display (char *, t_irc_ignore *);
extern int weechat_cmd_ignore (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_key (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_plugin (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_save (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_server (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_set (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_unalias (t_irc_server *, t_irc_channel *, char *);
extern int weechat_cmd_unignore (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_upgrade (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_uptime (t_irc_server *, t_irc_channel *, int, char **);
extern int weechat_cmd_window (t_irc_server *, t_irc_channel *, int, char **);

#endif /* command.h */
