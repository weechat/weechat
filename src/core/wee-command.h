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

#include "../gui/gui-buffer.h"

#define MAX_ARGS 8192

struct command
{
    char *name;                     /* WeeChat (internal) command name       */
    char *description;              /* command description (for /help)       */
    char *arguments;                /* command arguments (for /help)         */
    char *arguments_description;    /* arguments description (for /help)     */
    char *completion_template;      /* template for completion               */
                                    /* NULL=no completion, ""=default (nick) */
    int min_arg, max_arg;           /* min & max number of arguments         */
    int conversion;                 /* = 1 if cmd args are converted (charset*/
                                    /* and color) before execution           */
    int (*cmd_function)(struct t_gui_buffer *, char *, int, char **);
                                    /* function called when user enters cmd  */
};

extern struct command weechat_commands[];
struct t_weelist *weechat_index_commands;
struct t_weelist *weechat_last_index_command;

extern int command_command_is_used (char *);
extern void command_index_build ();
extern void command_index_free ();
extern void command_index_add (char *);
extern void command_index_remove (char *);
extern int command_is_command (char *);
extern void command_print_stdout (struct command *);

extern int command_alias (struct t_gui_buffer *, char *, int, char **);
extern int command_buffer (struct t_gui_buffer *, char *, int, char **);
extern int command_builtin (struct t_gui_buffer *, char *, int, char **);
extern int command_clear (struct t_gui_buffer *, char *, int, char **);
extern int command_debug (struct t_gui_buffer *, char *, int, char **);
extern int command_help (struct t_gui_buffer *, char *, int, char **);
extern int command_history (struct t_gui_buffer *, char *, int, char **);
extern int command_key (struct t_gui_buffer *, char *, int, char **);
extern int command_panel (struct t_gui_buffer *, char *, int, char **);
extern int command_plugin (struct t_gui_buffer *, char *, int, char **);
extern int command_quit (struct t_gui_buffer *, char *, int, char **);
extern int command_save (struct t_gui_buffer *, char *, int, char **);
extern int command_set (struct t_gui_buffer *, char *, int, char **);
extern int command_setp (struct t_gui_buffer *, char *, int, char **);
extern int command_unalias (struct t_gui_buffer *, char *, int, char **);
extern int command_unignore (struct t_gui_buffer *, char *, int, char **);
extern int command_upgrade (struct t_gui_buffer *, char *, int, char **);
extern int command_uptime (struct t_gui_buffer *, char *, int, char **);
extern int command_window (struct t_gui_buffer *, char *, int, char **);

#endif /* wee-command.h */
