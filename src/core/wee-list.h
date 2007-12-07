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


#ifndef __WEECHAT_LIST_H
#define __WEECHAT_LIST_H 1

#define WEELIST_POS_SORT      0
#define WEELIST_POS_BEGINNING 1
#define WEELIST_POS_END       2

struct t_weelist_item
{
    char *data;                        /* item data                         */
    struct t_weelist_item *prev_item;  /* link to previous item             */
    struct t_weelist_item *next_item;  /* link to next item                 */
};

struct t_weelist
{
    struct t_weelist_item *items;      /* items in list                     */
    struct t_weelist_item *last_item;  /* last item in list                 */
    int size;                          /* number of items in list           */
};

extern struct t_weelist *weelist_new ();
extern struct t_weelist_item *weelist_add (struct t_weelist *, char *, int);
extern struct t_weelist_item *weelist_search (struct t_weelist *, char *);
extern struct t_weelist_item *weelist_casesearch (struct t_weelist *, char *);
extern struct t_weelist_item *weelist_get (struct t_weelist *, int);
extern void weelist_remove (struct t_weelist *, struct t_weelist_item *);
extern void weelist_remove_all (struct t_weelist *);
extern void weelist_free (struct t_weelist *);
extern void weelist_print_log (struct t_weelist *, char *);

#endif /* wee-list.h */
