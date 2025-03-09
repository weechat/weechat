/*
 * relay-weechat-nicklist.c - nicklist functions for WeeChat protocol
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "relay-weechat.h"
#include "relay-weechat-nicklist.h"


/*
 * Builds a new nicklist structure (to store nicklist diffs).
 *
 * Returns pointer to new nicklist structure, NULL if error.
 */

struct t_relay_weechat_nicklist *
relay_weechat_nicklist_new (void)
{
    struct t_relay_weechat_nicklist *new_nicklist;

    new_nicklist = malloc (sizeof (*new_nicklist));
    if (!new_nicklist)
        return NULL;

    new_nicklist->nicklist_count = 0;
    new_nicklist->items_count = 0;
    new_nicklist->items = NULL;

    return new_nicklist;
}

/*
 * Adds a nicklist item in nicklist structure.
 */

void
relay_weechat_nicklist_add_item (struct t_relay_weechat_nicklist *nicklist,
                                 char diff, struct t_gui_nick_group *group,
                                 struct t_gui_nick *nick)
{
    struct t_relay_weechat_nicklist_item *new_items, *ptr_item;
    struct t_hdata *hdata;
    const char *str;
    int i;

    /*
     * check if the last "parent_group" (with diff = '^') of items is the same
     * as this one: if yes, don't add this parent group
     */
    if ((diff == RELAY_WEECHAT_NICKLIST_DIFF_PARENT)
        && (nicklist->items_count > 0))
    {
        for (i = nicklist->items_count - 1; i >= 0; i--)
        {
            if (nicklist->items[i].diff == RELAY_WEECHAT_NICKLIST_DIFF_PARENT)
            {
                if (nicklist->items[i].pointer == group)
                    return;
                break;
            }
        }
    }

    new_items = realloc (nicklist->items,
                         (nicklist->items_count + 1) * sizeof (new_items[0]));
    if (!new_items)
        return;

    nicklist->items = new_items;
    ptr_item = &(nicklist->items[nicklist->items_count]);
    if (group)
    {
        hdata = relay_hdata_nick_group;
        ptr_item->pointer = group;
    }
    else
    {
        hdata = relay_hdata_nick;
        ptr_item->pointer = nick;
    }
    ptr_item->diff = diff;
    ptr_item->group = (group) ? 1 : 0;
    ptr_item->visible = weechat_hdata_integer (hdata, ptr_item->pointer, "visible");
    ptr_item->level = (group) ? weechat_hdata_integer (hdata, ptr_item->pointer, "level") : 0;
    str = weechat_hdata_string (hdata, ptr_item->pointer, "name");
    ptr_item->name = (str) ? strdup (str) : NULL;
    str = weechat_hdata_string (hdata, ptr_item->pointer, "color");
    ptr_item->color = (str) ? strdup (str) : NULL;
    str = weechat_hdata_string (hdata, ptr_item->pointer, "prefix");
    ptr_item->prefix = (str) ? strdup (str) : NULL;
    str = weechat_hdata_string (hdata, ptr_item->pointer, "prefix_color");
    ptr_item->prefix_color = (str) ? strdup (str) : NULL;

    nicklist->items_count++;
}

/*
 * Frees a nicklist_item structure.
 */

void
relay_weechat_nicklist_item_free (struct t_relay_weechat_nicklist_item *item)
{
    if (!item)
        return;

    free (item->name);
    free (item->color);
    free (item->prefix);
    free (item->prefix_color);
}

/*
 * Frees a new nicklist structure.
 */

void
relay_weechat_nicklist_free (struct t_relay_weechat_nicklist *nicklist)
{
    int i;

    if (!nicklist)
        return;

    /* free items */
    if (nicklist->items_count > 0)
    {
        for (i = 0; i < nicklist->items_count; i++)
        {
            relay_weechat_nicklist_item_free (&(nicklist->items[i]));
        }
        free (nicklist->items);
    }

    free (nicklist);
}
