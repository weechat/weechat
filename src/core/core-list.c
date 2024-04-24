/*
 * core-list.c - sorted lists
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "weechat.h"
#include "core-list.h"
#include "core-log.h"
#include "core-string.h"
#include "../plugins/plugin.h"


/*
 * Creates a new list.
 *
 * Returns pointer to new list, NULL if error.
 */

struct t_weelist *
weelist_new ()
{
    struct t_weelist *new_weelist;

    new_weelist = malloc (sizeof (*new_weelist));
    if (new_weelist)
    {
        new_weelist->items = NULL;
        new_weelist->last_item = NULL;
        new_weelist->size = 0;
    }
    return new_weelist;
}

/*
 * Searches for position of data (to keep list sorted).
 */

struct t_weelist_item *
weelist_find_pos (struct t_weelist *weelist, const char *data)
{
    struct t_weelist_item *ptr_item;

    if (!weelist || !data)
        return NULL;

    for (ptr_item = weelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (string_strcasecmp (data, ptr_item->data) < 0)
            return ptr_item;
    }
    /* position not found, best position is at the end */
    return NULL;
}

/*
 * Inserts an element in the list (keeping list sorted).
 */

void
weelist_insert (struct t_weelist *weelist, struct t_weelist_item *item,
                const char *where)
{
    struct t_weelist_item *pos_item;

    if (!weelist || !item)
        return;

    if (weelist->items)
    {
        /* remove element if already in list */
        pos_item = weelist_search (weelist, item->data);
        if (pos_item)
            weelist_remove (weelist, pos_item);
    }

    if (weelist->items)
    {
        /* search position for new element, according to pos asked */
        pos_item = NULL;
        if (string_strcmp (where, WEECHAT_LIST_POS_BEGINNING) == 0)
            pos_item = weelist->items;
        else if (string_strcmp (where, WEECHAT_LIST_POS_END) == 0)
            pos_item = NULL;
        else
            pos_item = weelist_find_pos (weelist, item->data);

        if (pos_item)
        {
            /* insert data into the list (before position found) */
            item->prev_item = pos_item->prev_item;
            item->next_item = pos_item;
            if (pos_item->prev_item)
                (pos_item->prev_item)->next_item = item;
            else
                weelist->items = item;
            pos_item->prev_item = item;
        }
        else
        {
            /* add data to the end */
            item->prev_item = weelist->last_item;
            item->next_item = NULL;
            (weelist->last_item)->next_item = item;
            weelist->last_item = item;
        }
    }
    else
    {
        item->prev_item = NULL;
        item->next_item = NULL;
        weelist->items = item;
        weelist->last_item = item;
    }
}

/*
 * Creates new data and add it to the list.
 *
 * Returns pointer to new item, NULL if error.
 */

struct t_weelist_item *
weelist_add (struct t_weelist *weelist, const char *data, const char *where,
             void *user_data)
{
    struct t_weelist_item *new_item;

    if (!weelist || !data || !data[0] || !where || !where[0])
        return NULL;

    new_item = malloc (sizeof (*new_item));
    if (new_item)
    {
        new_item->data = strdup (data);
        new_item->user_data = user_data;
        weelist_insert (weelist, new_item, where);
        weelist->size++;
    }
    return new_item;
}

/*
 * Searches for data in a list (case sensitive).
 *
 * Returns pointer to item found, NULL if not found.
 */

struct t_weelist_item *
weelist_search (struct t_weelist *weelist, const char *data)
{
    struct t_weelist_item *ptr_item;

    if (!weelist || !data)
        return NULL;

    for (ptr_item = weelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (strcmp (data, ptr_item->data) == 0)
            return ptr_item;
    }
    /* data not found in list */
    return NULL;
}

/*
 * Searches for data in a list (case sensitive).
 *
 * Returns position of item found (>= 0), -1 if not found.
 */

int
weelist_search_pos (struct t_weelist *weelist, const char *data)
{
    struct t_weelist_item *ptr_item;
    int i;

    if (!weelist || !data)
        return -1;

    i = 0;
    for (ptr_item = weelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (strcmp (data, ptr_item->data) == 0)
            return i;
        i++;
    }
    /* data not found in list */
    return -1;
}

/*
 * Searches for data in a list (case insensitive).
 *
 * Returns pointer to item found, NULL if not found.
 */

struct t_weelist_item *
weelist_casesearch (struct t_weelist *weelist, const char *data)
{
    struct t_weelist_item *ptr_item;

    if (!weelist || !data)
        return NULL;

    for (ptr_item = weelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (string_strcasecmp (data, ptr_item->data) == 0)
            return ptr_item;
    }
    /* data not found in list */
    return NULL;
}

/*
 * Searches for data in a list (case insensitive).
 *
 * Returns position of item found (>= 0), -1 if not found.
 */

int
weelist_casesearch_pos (struct t_weelist *weelist, const char *data)
{
    struct t_weelist_item *ptr_item;
    int i;

    if (!weelist || !data)
        return -1;

    i = 0;
    for (ptr_item = weelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (string_strcasecmp (data, ptr_item->data) == 0)
            return i;
        i++;
    }
    /* data not found in list */
    return -1;
}

/*
 * Gets an item in a list by position (0 is first element).
 */

struct t_weelist_item *
weelist_get (struct t_weelist *weelist, int position)
{
    int i;
    struct t_weelist_item *ptr_item;

    if (!weelist)
        return NULL;

    i = 0;
    ptr_item = weelist->items;
    while (ptr_item)
    {
        if (i == position)
            return ptr_item;
        ptr_item = ptr_item->next_item;
        i++;
    }
    /* item not found */
    return NULL;
}

/*
 * Sets a new value for an item.
 */

void
weelist_set (struct t_weelist_item *item, const char *value)
{
    if (!item || !value)
        return;

    free (item->data);
    item->data = strdup (value);
}

/*
 * Gets next item.
 *
 * Returns NULL if end of list has been reached.
 */

struct t_weelist_item *
weelist_next (struct t_weelist_item *item)
{
    if (item)
        return item->next_item;

    return NULL;
}

/*
 * Gets previous item.
 *
 * Returns NULL if beginning of list has been reached.
 */

struct t_weelist_item *
weelist_prev (struct t_weelist_item *item)
{
    if (item)
        return item->prev_item;

    return NULL;
}

/*
 * Gets string pointer to item data.
 */

const char *
weelist_string (struct t_weelist_item *item)
{
    if (item)
        return item->data;

    return NULL;
}

/*
 * Gets user data pointer to item data.
 */

void *
weelist_user_data (struct t_weelist_item *item)
{
    if (item)
        return item->user_data;

    return NULL;
}

/*
 * Gets size of list.
 */

int
weelist_size (struct t_weelist *weelist)
{
    if (weelist)
        return weelist->size;

    return 0;
}

/*
 * Removes an item from a list.
 */

void
weelist_remove (struct t_weelist *weelist, struct t_weelist_item *item)
{
    struct t_weelist_item *new_items;

    if (!weelist || !item)
        return;

    /* remove item from list */
    if (weelist->last_item == item)
        weelist->last_item = item->prev_item;
    if (item->prev_item)
    {
        (item->prev_item)->next_item = item->next_item;
        new_items = weelist->items;
    }
    else
        new_items = item->next_item;

    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;

    /* free data */
    free (item->data);
    free (item);
    weelist->items = new_items;

    weelist->size--;
}

/*
 * Removes all items from a list.
 */

void
weelist_remove_all (struct t_weelist *weelist)
{
    if (!weelist)
        return;

    while (weelist->items)
    {
        weelist_remove (weelist, weelist->items);
    }
}

/*
 * Frees a list.
 */

void
weelist_free (struct t_weelist *weelist)
{
    if (!weelist)
        return;

    weelist_remove_all (weelist);
    free (weelist);
}

/*
 * Prints list in WeeChat log file (usually for crash dump).
 */

void
weelist_print_log (struct t_weelist *weelist, const char *name)
{
    struct t_weelist_item *ptr_item;
    int i;

    log_printf ("[weelist %s (addr:%p)]", name, weelist);
    log_printf ("  items. . . . . . . . . : %p", weelist->items);
    log_printf ("  last_item. . . . . . . : %p", weelist->last_item);
    log_printf ("  size . . . . . . . . . : %d", weelist->size);

    i = 0;
    for (ptr_item = weelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        log_printf ("  [item %d (addr:%p)]", i, ptr_item);
        log_printf ("    data . . . . . . . . : '%s'", ptr_item->data);
        log_printf ("    user_data. . . . . . : %p", ptr_item->user_data);
        log_printf ("    prev_item. . . . . . : %p", ptr_item->prev_item);
        log_printf ("    next_item. . . . . . : %p", ptr_item->next_item);
        i++;
    }
}
