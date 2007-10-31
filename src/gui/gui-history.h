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


#ifndef __WEECHAT_HISTORY_H
#define __WEECHAT_HISTORY_H 1

typedef struct t_history t_history;

struct t_history
{
    char *text;                 /* text or command (as entered by user)     */
    t_history *next_history;    /* link to next text/command                */
    t_history *prev_history;    /* link to previous text/command            */
};

extern t_history *history_global;
extern t_history *history_global_last;
extern t_history *history_global_ptr;

extern void history_buffer_add (void *, char *);
extern void history_global_add (char *);
extern void history_global_free ();
extern void history_buffer_free (void *);

#endif /* history.h */
