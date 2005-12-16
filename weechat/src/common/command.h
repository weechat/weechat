/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
    int (*cmd_function_args)(t_gui_window *, int, char **);
                                    /* function called when user enters cmd   */
    int (*cmd_function_1arg)(t_gui_window *, char *);
                                    /* function called when user enters cmd   */
};

typedef struct t_weechat_alias t_weechat_alias;

struct t_weechat_alias
{
    char *alias_name;
    char *alias_command;
    t_weechat_alias *prev_alias;
    t_weechat_alias *next_alias;
};

extern t_weechat_command weechat_commands[];

extern t_weechat_alias *weechat_alias;
extern t_weelist *index_commands;
extern t_weelist *last_index_command;

extern void command_index_build ();
extern void command_index_free ();
extern t_weechat_alias *alias_new (char *, char *);
extern void alias_free_all ();
extern char **explode_string (char *, char *, int, int *);
extern void free_exploded_string (char **);
extern int exec_weechat_command (t_gui_window *, t_irc_server *, char *);
extern void user_command (t_gui_window *, t_irc_server *, char *);
extern int weechat_cmd_alias (t_gui_window *, char *);
extern int weechat_cmd_buffer (t_gui_window *, int, char **);
extern int weechat_cmd_charset (t_gui_window *, int, char **);
extern int weechat_cmd_clear (t_gui_window *, int, char **);
extern int weechat_cmd_connect (t_gui_window *, int, char **);
extern int weechat_cmd_debug (t_gui_window *, int, char **);
extern int weechat_cmd_disconnect (t_gui_window *, int, char **);
extern int weechat_cmd_help (t_gui_window *, int, char **);
extern int weechat_cmd_history (t_gui_window *, int, char **);
extern void weechat_cmd_ignore_display (char *, t_irc_ignore *);
extern int weechat_cmd_ignore (t_gui_window *, int, char **);
extern int weechat_cmd_key (t_gui_window *, char *);
extern int weechat_cmd_plugin (t_gui_window *, int, char **);
extern int weechat_cmd_save (t_gui_window *, int, char **);
extern int weechat_cmd_server (t_gui_window *, int, char **);
extern int weechat_cmd_set (t_gui_window *, char *);
extern int weechat_cmd_unalias (t_gui_window *, char *);
extern int weechat_cmd_unignore (t_gui_window *, int, char **);
extern int weechat_cmd_upgrade (t_gui_window *, int, char **);
extern int weechat_cmd_uptime (t_gui_window *, int, char **);
extern int weechat_cmd_window (t_gui_window *, int, char **);

#endif /* command.h */
