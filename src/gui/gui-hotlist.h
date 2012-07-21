/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_GUI_HOTLIST_H
#define __WEECHAT_GUI_HOTLIST_H 1

enum t_gui_hotlist_priority
{
    GUI_HOTLIST_LOW = 0,
    GUI_HOTLIST_MESSAGE,
    GUI_HOTLIST_PRIVATE,
    GUI_HOTLIST_HIGHLIGHT,
    /* number of priorities */
    GUI_HOTLIST_NUM_PRIORITIES,
};
#define GUI_HOTLIST_NUM_PRIORITIES_STR "4"

#define GUI_HOTLIST_MIN 0
#define GUI_HOTLIST_MAX (GUI_HOTLIST_NUM_PRIORITIES - 1)

struct t_gui_hotlist
{
    enum t_gui_hotlist_priority priority;  /* 0=crappy msg (join/part),     */
                                           /* 1=msg, 2=pv, 3=nick highlight */
    struct timeval creation_time;          /* time when entry was added     */
    struct t_gui_buffer *buffer;           /* associated buffer             */
    int count[GUI_HOTLIST_NUM_PRIORITIES]; /* number of msgs by priority    */
    struct t_gui_hotlist *prev_hotlist;    /* link to previous hotlist      */
    struct t_gui_hotlist *next_hotlist;    /* link to next hotlist          */
};

/* history variables */

extern struct t_gui_hotlist *gui_hotlist;
extern struct t_gui_hotlist *last_gui_hotlist;
extern struct t_gui_buffer *gui_hotlist_initial_buffer;
extern int gui_add_hotlist;

/* hotlist functions */

extern struct t_gui_hotlist *gui_hotlist_add (struct t_gui_buffer *buffer,
                                              enum t_gui_hotlist_priority priority,
                                              struct timeval *creation_time);
extern void gui_hotlist_resort ();
extern void gui_hotlist_clear ();
extern void gui_hotlist_remove_buffer (struct t_gui_buffer *buffer);
extern struct t_hdata *gui_hotlist_hdata_hotlist_cb (void *data,
                                                     const char *hdata_name);
extern int gui_hotlist_add_to_infolist (struct t_infolist *infolist,
                                        struct t_gui_hotlist *hotlist);
extern void gui_hotlist_print_log ();

#endif /* __WEECHAT_GUI_HOTLIST_H */
