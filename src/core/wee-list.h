/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_LIST_H
#define WEECHAT_LIST_H

struct t_weelist_item
{
    char *data;                        /* item data                         */
    void *user_data;                   /* pointer to user data              */
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
extern struct t_weelist_item *weelist_add (struct t_weelist *weelist,
                                           const char *data, const char *where,
                                           void *user_data);
extern struct t_weelist_item *weelist_search (struct t_weelist *weelist,
                                              const char *data);
extern int weelist_search_pos (struct t_weelist *weelist, const char *data);
extern struct t_weelist_item *weelist_casesearch (struct t_weelist *weelist,
                                                  const char *data);
extern int weelist_casesearch_pos (struct t_weelist *weelist, const char *data);
extern struct t_weelist_item *weelist_get (struct t_weelist *weelist,
                                           int position);
extern void weelist_set (struct t_weelist_item *item, const char *value);
extern struct t_weelist_item *weelist_next (struct t_weelist_item *item);
extern struct t_weelist_item *weelist_prev (struct t_weelist_item *item);
extern const char *weelist_string (struct t_weelist_item *item);
extern void *weelist_user_data (struct t_weelist_item *item);
extern int weelist_size (struct t_weelist *weelist);
extern void weelist_remove (struct t_weelist *weelist,
                            struct t_weelist_item *item);
extern void weelist_remove_all (struct t_weelist *weelist);
extern void weelist_free (struct t_weelist *weelist);
extern void weelist_print_log (struct t_weelist *weelist, const char *name);

#endif /* WEECHAT_LIST_H */
