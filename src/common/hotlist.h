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


#ifndef __WEECHAT_HOTLIST_H
#define __WEECHAT_HOTLIST_H 1

#include "../protocols/irc/irc.h"

#define HOTLIST_LOW       0
#define HOTLIST_MSG       1
#define HOTLIST_PRIVATE   2
#define HOTLIST_HIGHLIGHT 3

typedef struct t_weechat_hotlist t_weechat_hotlist;

struct t_weechat_hotlist
{
    int priority;                       /* 0=crappy msg (join/part), 1=msg, */
                                        /* 2=pv, 3=nick highlight           */
    struct timeval creation_time;       /* time when entry was added        */
    t_irc_server *server;               /* associated server                */
    t_gui_buffer *buffer;               /* associated buffer                */
    t_weechat_hotlist *prev_hotlist;    /* link to previous hotlist         */
    t_weechat_hotlist *next_hotlist;    /* link to next hotlist             */
};

extern t_weechat_hotlist *weechat_hotlist;
extern t_weechat_hotlist *last_weechat_hotlist;
extern t_gui_buffer *hotlist_initial_buffer;

extern void hotlist_add (int, struct timeval *, t_irc_server *, t_gui_buffer *, int);
extern void hotlist_resort ();
extern void hotlist_free (t_weechat_hotlist **, t_weechat_hotlist **, t_weechat_hotlist *);
extern void hotlist_free_all (t_weechat_hotlist **, t_weechat_hotlist **);
extern void hotlist_remove_buffer (t_gui_buffer *);
extern void hotlist_print_log ();

#endif /* hotlist.h */
