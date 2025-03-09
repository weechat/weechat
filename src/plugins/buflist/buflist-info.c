/*
 * buflist-info.c - infolist hook for buflist plugin
 *
 * Copyright (C) 2019 Simmo Saan <simmo.saan@gmail.com>
 * Copyright (C) 2019-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"


/*
 * Adds a buffer in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
buflist_buffer_add_to_infolist (struct t_infolist *infolist, struct t_gui_buffer *buffer)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !buffer)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", buffer))
        return 0;

    return 1;
}

/*
 * Returns infolist "buflist".
 */

struct t_infolist *
buflist_info_infolist_buflist_cb (const void *pointer, void *data,
                                  const char *infolist_name,
                                  void *obj_pointer, const char *arguments)
{
    int item_index, i, size;
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;
    void *gui_buffers;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;

    if (arguments && arguments[0])
    {
        item_index = buflist_bar_item_get_index (arguments);
        if (item_index < 0)
            return NULL;
    }
    else
        item_index = 0;

    if (!buflist_list_buffers[item_index])
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    gui_buffers = weechat_hdata_get_list (buflist_hdata_buffer, "gui_buffers");

    /* build list with all buffers in buflist */
    size = weechat_arraylist_size (buflist_list_buffers[item_index]);
    for (i = 0; i < size; i++)
    {
        ptr_buffer = weechat_arraylist_get (buflist_list_buffers[item_index], i);

        /* check if ptr_buffer is still valid (buffer not closed) */
        if (weechat_hdata_check_pointer (buflist_hdata_buffer,
                                         gui_buffers, ptr_buffer))
        {
            if (!buflist_buffer_add_to_infolist (ptr_infolist, ptr_buffer))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
    }
    return ptr_infolist;
}

/*
 * Hooks infolist for buflist plugin.
 */

void
buflist_info_init (void)
{
    weechat_hook_infolist (
        "buflist", N_("list of buffers in a buflist bar item"),
        NULL,
        N_("buflist bar item name (optional)"),
        &buflist_info_infolist_buflist_cb, NULL, NULL);
}
