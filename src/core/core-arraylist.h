/*
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_ARRAYLIST_H
#define WEECHAT_ARRAYLIST_H

struct t_arraylist;

typedef int (t_arraylist_cmp)(void *data, struct t_arraylist *arraylist,
                              void *pointer1, void *pointer2);
typedef void (t_arraylist_free)(void *data, struct t_arraylist *arraylist,
                                void *pointer);

struct t_arraylist
{
    int size;                          /* number of items in data           */
    int size_alloc;                    /* number of allocated items         */
    int size_alloc_min;                /* min number of allocated items     */
    int sorted;                        /* 1 if the arraylist is sorted      */
    int allow_duplicates;              /* 1 if duplicates are allowed       */
    void **data;                       /* pointers to data                  */
    t_arraylist_cmp *callback_cmp;     /* compare two elements              */
    void *callback_cmp_data;           /* data for compare callback         */
    t_arraylist_free *callback_free;   /* free an element                   */
    void *callback_free_data;          /* data for free callback            */
};

extern struct t_arraylist *arraylist_new (int initial_size,
                                          int sorted,
                                          int allow_duplicates,
                                          t_arraylist_cmp *callback_cmp,
                                          void *callback_cmp_data,
                                          t_arraylist_free *callback_free,
                                          void *callback_free_data);
extern int arraylist_size (struct t_arraylist *arraylist);
extern void *arraylist_get (struct t_arraylist *arraylist, int index);
extern void *arraylist_search (struct t_arraylist *arraylist, void *pointer,
                               int *index, int *index_insert);
extern int arraylist_insert (struct t_arraylist *arraylist, int index,
                             void *pointer);
extern int arraylist_add (struct t_arraylist *arraylist, void *pointer);
extern int arraylist_remove (struct t_arraylist *arraylist, int index);
extern int arraylist_clear (struct t_arraylist *arraylist);
extern void arraylist_free (struct t_arraylist *arraylist);
extern void arraylist_print_log (struct t_arraylist *arraylist,
                                 const char *name);

#endif /* WEECHAT_ARRAYLIST_H */
