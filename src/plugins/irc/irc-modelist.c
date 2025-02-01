/*
 * irc-modelist.c - channel mode list management for IRC plugin
 *
 * Copyright (C) 2015 Simmo Saan <simmo.saan@gmail.com>
 * Copyright (C) 2018-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-modelist.h"


/*
 * Checks if a modelist pointer is valid for a channel.
 *
 * Returns:
 *   1: modelist exists for channel
 *   0: modelist does not exist for channel
 */

int
irc_modelist_valid (struct t_irc_channel *channel,
                    struct t_irc_modelist *modelist)
{
    struct t_irc_modelist *ptr_modelist;

    if (!channel || !modelist)
        return 0;

    for (ptr_modelist = channel->modelists; ptr_modelist;
         ptr_modelist = ptr_modelist->next_modelist)
    {
        if (ptr_modelist == modelist)
            return 1;
    }

    /* modelist not found */
    return 0;
}

/*
 * Searches for a modelist by type.
 *
 * Returns pointer to modelist found, NULL if not found.
 */

struct t_irc_modelist *
irc_modelist_search (struct t_irc_channel *channel, char type)
{
    struct t_irc_modelist *ptr_modelist;

    if (!channel)
        return NULL;

    for (ptr_modelist = channel->modelists; ptr_modelist;
         ptr_modelist = ptr_modelist->next_modelist)
    {
        if (ptr_modelist->type == type)
            return ptr_modelist;
    }
    return NULL;
}

/*
 * Creates a new modelist in a channel.
 *
 * Returns pointer to new modelist, NULL if error.
 */

struct t_irc_modelist *
irc_modelist_new (struct t_irc_channel *channel, char type)
{
    struct t_irc_modelist *new_modelist;

    /* alloc memory for new modelist */
    if ((new_modelist = malloc (sizeof (*new_modelist))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot allocate new modelist"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }

    /* initialize new modelist */
    new_modelist->type = type;
    new_modelist->state = IRC_MODELIST_STATE_EMPTY;
    new_modelist->items = NULL;
    new_modelist->last_item = NULL;

    /* add new modelist to channel */
    new_modelist->prev_modelist = channel->last_modelist;
    new_modelist->next_modelist = NULL;
    if (channel->modelists)
        (channel->last_modelist)->next_modelist = new_modelist;
    else
        channel->modelists = new_modelist;
    channel->last_modelist = new_modelist;

    /* all is OK, return address of new modelist */
    return new_modelist;
}

/*
 * Frees a modelist and remove it from channel.
 */

void
irc_modelist_free (struct t_irc_channel *channel,
                   struct t_irc_modelist *modelist)
{
    struct t_irc_modelist *new_modelists;

    if (!channel || !modelist)
        return;

    /* remove modelist from channel modelists */
    if (channel->last_modelist == modelist)
        channel->last_modelist = modelist->prev_modelist;
    if (modelist->prev_modelist)
    {
        (modelist->prev_modelist)->next_modelist = modelist->next_modelist;
        new_modelists = channel->modelists;
    }
    else
        new_modelists = modelist->next_modelist;

    if (modelist->next_modelist)
        (modelist->next_modelist)->prev_modelist = modelist->prev_modelist;

    /* free linked lists */
    irc_modelist_item_free_all (modelist);

    free (modelist);

    channel->modelists = new_modelists;
}

/*
 * Frees all modelists for a channel.
 */

void
irc_modelist_free_all (struct t_irc_channel *channel)
{
    while (channel->modelists)
    {
        irc_modelist_free (channel, channel->modelists);
    }
}

/*
 * Checks if a modelist item pointer is valid for a modelist.
 *
 * Returns:
 *   1: item exists for modelist
 *   0: item does not exist for modelist
 */

int
irc_modelist_item_valid (struct t_irc_modelist *modelist,
                         struct t_irc_modelist_item *item)
{
    struct t_irc_modelist_item *ptr_item;

    if (!modelist || !item)
        return 0;

    for (ptr_item = modelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (ptr_item == item)
            return 1;
    }

    /* item not found */
    return 0;
}

/*
 * Searches for an item by mask.
 *
 * Returns pointer to item found, NULL if not found.
 */

struct t_irc_modelist_item *
irc_modelist_item_search_mask (struct t_irc_modelist *modelist, const char *mask)
{
    struct t_irc_modelist_item *ptr_item;

    if (!modelist || !mask)
        return NULL;

    for (ptr_item = modelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (strcmp (ptr_item->mask, mask) == 0)
            return ptr_item;
    }
    return NULL;
}

/*
 * Searches for an item by number.
 *
 * Returns pointer to item found, NULL if not found.
 */

struct t_irc_modelist_item *
irc_modelist_item_search_number (struct t_irc_modelist *modelist, int number)
{
    struct t_irc_modelist_item *ptr_item;

    if (!modelist)
        return NULL;

    for (ptr_item = modelist->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        if (ptr_item->number == number)
            return ptr_item;
    }
    return NULL;
}

/*
 * Creates a new item in a modelist.
 *
 * Returns pointer to new item, NULL if error.
 */

struct t_irc_modelist_item *
irc_modelist_item_new (struct t_irc_modelist *modelist,
                       const char *mask, const char *setter, time_t datetime)
{
    struct t_irc_modelist_item *new_item;

    if (!mask)
        return NULL;

    /* alloc memory for new item */
    if ((new_item = malloc (sizeof (*new_item))) == NULL)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot allocate new modelist item"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME);
        return NULL;
    }

    /* initialize new item */
    new_item->number = (modelist->last_item) ?
        modelist->last_item->number + 1 : 0;
    new_item->mask = strdup (mask);
    new_item->setter = (setter) ? strdup (setter) : NULL;
    new_item->datetime = datetime;

    /* add new item to modelist */
    new_item->prev_item = modelist->last_item;
    new_item->next_item = NULL;
    if (modelist->items)
        (modelist->last_item)->next_item = new_item;
    else
        modelist->items = new_item;
    modelist->last_item = new_item;

    if ((modelist->state == IRC_MODELIST_STATE_EMPTY) ||
        (modelist->state == IRC_MODELIST_STATE_RECEIVED))
    {
        modelist->state = IRC_MODELIST_STATE_MODIFIED;
    }

    /* all is OK, return address of new item */
    return new_item;
}

/*
 * Frees an item and remove it from modelist.
 */

void
irc_modelist_item_free (struct t_irc_modelist *modelist,
                        struct t_irc_modelist_item *item)
{
    struct t_irc_modelist_item *new_items;

    if (!modelist || !item)
        return;

    /* remove item from modelist list */
    if (modelist->last_item == item)
        modelist->last_item = item->prev_item;
    if (item->prev_item)
    {
        (item->prev_item)->next_item = item->next_item;
        new_items = modelist->items;
    }
    else
        new_items = item->next_item;

    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;

    /* free item data */
    free (item->mask);
    free (item->setter);
    free (item);

    modelist->items = new_items;

    if (modelist->state == IRC_MODELIST_STATE_RECEIVED)
        modelist->state = IRC_MODELIST_STATE_MODIFIED;
}

/*
 * Frees all items for a modelist.
 */

void
irc_modelist_item_free_all (struct t_irc_modelist *modelist)
{
    while (modelist->items)
    {
        irc_modelist_item_free (modelist, modelist->items);
    }
    modelist->state = IRC_MODELIST_STATE_EMPTY;
}

/*
 * Returns hdata for modelist item.
 */

struct t_hdata *
irc_modelist_hdata_item_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_item", "next_item",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_modelist_item, number, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_modelist_item, mask, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_modelist_item, setter, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_modelist_item, datetime, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_modelist_item, prev_item, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_modelist_item, next_item, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Returns hdata for modelist.
 */

struct t_hdata *
irc_modelist_hdata_modelist_cb (const void *pointer, void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_modelist", "next_modelist",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_modelist, type, CHAR, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_modelist, state, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_modelist, items, POINTER, 0, NULL, "irc_modelist_item");
        WEECHAT_HDATA_VAR(struct t_irc_modelist, last_item, POINTER, 0, NULL, "irc_modelist_item");
        WEECHAT_HDATA_VAR(struct t_irc_modelist, prev_modelist, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_modelist, next_modelist, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a modelist item in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_modelist_item_add_to_infolist (struct t_infolist *infolist,
                                   struct t_irc_modelist_item *item)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !item)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_integer (ptr_item, "number", item->number))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "mask", item->mask))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "setter", item->setter))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "datetime", item->datetime))
        return 0;

    return 1;
}

/*
 * Adds a modelist in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_modelist_add_to_infolist (struct t_infolist *infolist,
                              struct t_irc_modelist *modelist)
{
    struct t_infolist_item *ptr_item;
    char str_type[2];

    if (!infolist || !modelist)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    str_type[0] = modelist->type;
    str_type[1] = '\0';

    if (!weechat_infolist_new_var_string (ptr_item, "type", str_type))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "state", modelist->state))
        return 0;

    return 1;
}

/*
 * Prints modelist item infos in WeeChat log file (usually for crash dump).
 */

void
irc_modelist_item_print_log (struct t_irc_modelist_item *item)
{
    weechat_log_printf ("");
    weechat_log_printf ("      => modelist item %d (addr:%p):", item->number, item);
    weechat_log_printf ("           mask . . . . . . . . . . : '%s'", item->mask);
    weechat_log_printf ("           setter . . . . . . . . . : '%s'", item->setter);
    weechat_log_printf ("           datetime . . . . . . . . : %lld",
                        (long long)(item->datetime));
    weechat_log_printf ("           prev_item  . . . . . . . : %p", item->prev_item);
    weechat_log_printf ("           next_item  . . . . . . . : %p", item->next_item);
}

/*
 * Prints modelist infos in WeeChat log file (usually for crash dump).
 */

void
irc_modelist_print_log (struct t_irc_modelist *modelist)
{
    struct t_irc_modelist_item *ptr_item;

    weechat_log_printf ("");
    weechat_log_printf ("    => modelist \"%c\" (addr:%p):", modelist->type, modelist);
    weechat_log_printf ("         state. . . . . . . . . . : %d", modelist->state);
    weechat_log_printf ("         prev_modelist  . . . . . : %p", modelist->prev_modelist);
    weechat_log_printf ("         next_modelist  . . . . . : %p", modelist->next_modelist);
    for (ptr_item = modelist->items; ptr_item; ptr_item = ptr_item->next_item)
    {
        irc_modelist_item_print_log (ptr_item);
    }
}
