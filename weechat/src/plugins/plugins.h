/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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

#define PLUGIN_UNKNOWN 0
#define PLUGIN_PERL    1
#define PLUGIN_PYTHON  2
#define PLUGIN_RUBY    3

typedef struct t_plugin_handler t_plugin_handler;

struct t_plugin_handler
{
    int plugin_type;                /* plugin type (Perl, Python, Ruby)     */
    char *name;                     /* name (message or command)            */
    char *function_name;            /* name of function (handler)           */
    t_plugin_handler *prev_handler; /* link to previous handler             */
    t_plugin_handler *next_handler; /* link to next handler                 */
};


extern void plugins_init ();
extern void plugins_load (int, char *);
extern void plugins_unload (int, char *);
extern void plugins_msg_handler_add (int, char *, char *);
extern void plugins_event_msg (char *, char *);
extern void plugins_end ();

#endif /* plugins.h */
