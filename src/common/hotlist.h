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


#ifndef __WEECHAT_HOTLIST_H
#define __WEECHAT_HOTLIST_H 1

#include "../irc/irc.h"

typedef struct t_weechat_hotlist t_weechat_hotlist;

struct t_weechat_hotlist
{
    int priority;                       /* 0=crappy msg (join/part), 1=msg, */
                                        /* 2=nick highlight                 */
    t_gui_buffer *buffer;               /* associated buffer                */
    t_weechat_hotlist *prev_hotlist;    /* link to previous hotlist         */
    t_weechat_hotlist *next_hotlist;    /* link to next hotlist             */
};

extern t_weechat_hotlist *hotlist;
extern t_gui_buffer *hotlist_initial_buffer;

extern void hotlist_add (int, t_gui_buffer *);
extern void hotlist_free (t_weechat_hotlist *);
extern void hotlist_free_all ();
extern void hotlist_remove_buffer (t_gui_buffer *);

#endif /* hotlist.h */
