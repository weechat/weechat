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


#ifndef __WEECHAT_GUI_INFOBAR_H
#define __WEECHAT_GUI_INFOBAR_H 1

struct t_gui_infobar
{
    int color;                      /* text color                           */
    char *text;                     /* infobar text                         */
    int remaining_time;             /* delay (sec) before erasing this text */
                                    /* if < 0, text is never erased (except */
                                    /* by user action to erase it)          */
    struct t_gui_infobar *next_infobar; /* next message for infobar         */
};

/* infobar variables */

extern struct t_gui_infobar *gui_infobar;
extern struct t_hook *gui_infobar_refresh_timer;
extern struct t_hook *gui_infobar_highlight_timer;

/* infobar functions */

extern void gui_infobar_printf (int delay, int color,
                                const char *message, ...);
extern void gui_infobar_remove ();
extern void gui_infobar_remove_all ();

/* infobar functions (GUI dependent) */

extern void gui_infobar_draw_time (struct t_gui_buffer *buffer);
extern void gui_infobar_draw (struct t_gui_buffer *buffer, int erase);
extern int gui_infobar_refresh_timer_cb (void *data);
extern int gui_infobar_highlight_timer_cb (void *data);

#endif /* gui-infobar.h */
