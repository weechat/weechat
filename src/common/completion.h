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


#ifndef __WEECHAT_COMPLETION_H
#define __WEECHAT_COMPLETION_H 1

typedef struct t_completion t_completion;

struct t_completion
{
    char *base_word;            /* word to complete (when Tab was pressed)  */
    int base_word_pos;          /* beggining of base word                   */
    int position;               /* position where we shoud complete         */
    char *word_found;           /* word found (to replace base word)        */
    int position_replace;       /* position where word should be replaced   */
    int diff_size;              /* size difference (< 0 = char(s) deleted)  */
};

extern void completion_init (t_completion *);
extern void completion_free (t_completion *);
extern void completion_search (t_completion *, void *, char *, int, int);

#endif /* completion.h */
