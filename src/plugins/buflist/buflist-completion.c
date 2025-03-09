/*
 * buflist-completion.c - completion for buflist command
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"
#include "buflist-config.h"


/*
 * Adds all buflist items to completion list.
 */

int
buflist_completion_items_cb (const void *pointer, void *data,
                             const char *completion_item,
                             struct t_gui_buffer *buffer,
                             struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < BUFLIST_BAR_NUM_ITEMS; i++)
    {
        weechat_completion_list_add (completion,
                                     buflist_bar_item_get_name (i),
                                     0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds used buflist items to completion list.
 */

int
buflist_completion_items_used_cb (const void *pointer, void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < weechat_config_integer (buflist_config_look_use_items); i++)
    {
        weechat_completion_list_add (completion,
                                     buflist_bar_item_get_name (i),
                                     0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
buflist_completion_init (void)
{
    weechat_hook_completion ("buflist_items",
                             N_("buflist bar items"),
                             &buflist_completion_items_cb, NULL, NULL);
    weechat_hook_completion ("buflist_items_used",
                             N_("buflist bar items used (according to option "
                                "buflist.look.use_items)"),
                             &buflist_completion_items_used_cb, NULL, NULL);
}
