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


#ifndef __WEECHAT_PLUGINS_H
#define __WEECHAT_PLUGINS_H 1

#include "../gui/gui.h"

#define PLUGIN_TYPE_PERL    0
#define PLUGIN_TYPE_PYTHON  1
#define PLUGIN_TYPE_RUBY    2

typedef struct t_plugin_script t_plugin_script;

struct t_plugin_script
{
    char *name;                     /* name of script                       */
    char *version;                  /* version of script                    */
    char *shutdown_func;            /* function when script ends            */
    char *description;              /* description of script                */
    t_plugin_script *prev_script;   /* link to previous Perl script         */
    t_plugin_script *next_script;   /* link to next Perl script             */
};

typedef struct t_plugin_handler t_plugin_handler;

struct t_plugin_handler
{
    int plugin_type;                /* plugin type (Perl, Python, Ruby)     */
    char *name;                     /* name of IRC command (PRIVMSG, ..)
                                       or command (without first '/')       */
    char *function_name;            /* name of function (handler)           */
    int running;                    /* 1 if currently running               */
                                    /* (used to prevent circular call)      */
    t_plugin_handler *prev_handler; /* link to previous handler             */
    t_plugin_handler *next_handler; /* link to next handler                 */
};

extern t_plugin_handler *plugin_msg_handlers;
extern t_plugin_handler *last_plugin_msg_handler;

extern t_plugin_handler *plugin_cmd_handlers;
extern t_plugin_handler *last_plugin_cmd_handler;

#ifdef PLUGIN_PERL
extern t_plugin_script *perl_scripts;
#endif

#ifdef PLUGIN_PYTHON
extern t_plugin_script *python_scripts;
#endif

#ifdef PLUGIN_RUBY
extern t_plugin_script *ruby_scripts;
#endif

extern void plugin_auto_load (int, char *);
extern void plugin_init ();
extern void plugin_load (int, char *);
extern void plugin_unload (int, char *);
extern t_plugin_handler *plugin_handler_search (t_plugin_handler *, char *);
extern void plugin_handler_add (t_plugin_handler **, t_plugin_handler **,
                                int, char *, char *);
extern void plugin_handler_free_all_type (t_plugin_handler **,
                                          t_plugin_handler **, int);
extern void plugin_event_msg (char *, char *, char *);
extern int plugin_exec_command (char *, char *, char *);
extern t_gui_buffer *plugin_find_buffer (char *, char *);
extern void plugin_end ();

#endif /* plugins.h */
