/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_GUI_HOTLIST_H
#define __WEECHAT_GUI_HOTLIST_H 1

#define GUI_HOTLIST_LOW       0
#define GUI_HOTLIST_MESSAGE   1
#define GUI_HOTLIST_PRIVATE   2
#define GUI_HOTLIST_HIGHLIGHT 3

#define GUI_HOTLIST_MIN       0
#define GUI_HOTLIST_MAX       3

struct t_gui_hotlist
{
    int priority;                      /* 0=crappy msg (join/part), 1=msg,  */
                                       /* 2=pv, 3=nick highlight            */
    struct timeval creation_time;      /* time when entry was added         */
    struct t_gui_buffer *buffer;       /* associated buffer                 */
    struct t_gui_hotlist *prev_hotlist;/* link to previous hotlist          */
    struct t_gui_hotlist *next_hotlist;/* link to next hotlist              */
};

/* history variables */

extern struct t_gui_hotlist *gui_hotlist;
extern struct t_gui_hotlist *last_gui_hotlist;
extern struct t_gui_buffer *gui_hotlist_initial_buffer;
extern int gui_add_hotlist;

/* hotlist functions */

extern void gui_hotlist_add (struct t_gui_buffer *buffer, int priority,
                             struct timeval *creation_time,
                             int allow_current_buffer);
extern void gui_hotlist_resort ();
extern void gui_hotlist_free (struct t_gui_hotlist **hotlist,
                              struct t_gui_hotlist **last_hotlist,
                              struct t_gui_hotlist *ptr_hotlist);
extern void gui_hotlist_free_all (struct t_gui_hotlist **hotlist,
                                  struct t_gui_hotlist **last_hotlist);
extern void gui_hotlist_remove_buffer (struct t_gui_buffer *buffer);
extern int gui_hotlist_add_to_infolist (struct t_infolist *infolist,
                                        struct t_gui_hotlist *hotlist);
extern void gui_hotlist_print_log ();

#endif /* gui-hotlist.h */
