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


#ifndef __WEECHAT_GUI_HISTORY_H
#define __WEECHAT_GUI_HISTORY_H 1

struct t_gui_buffer;

struct t_gui_history
{
    char *text;                        /* text or command (entered by user) */
    struct t_gui_history *next_history;/* link to next text/command         */
    struct t_gui_history *prev_history;/* link to previous text/command     */
};

extern struct t_gui_history *history_global;
extern struct t_gui_history *history_global_last;
extern struct t_gui_history *history_global_ptr;

/* history functions (gui-history.c) */
extern void gui_history_buffer_add (struct t_gui_buffer *buffer, char *string);
extern void gui_history_global_add (char *string);
extern void gui_history_global_free ();
extern void gui_history_buffer_free (struct t_gui_buffer *buffer);

#endif /* gui-history.h */
