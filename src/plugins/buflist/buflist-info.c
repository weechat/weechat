/*
 * SPDX-FileCopyrightText: 2019 Simmo Saan <simmo.saan@gmail.com>
 * SPDX-FileCopyrightText: 2019-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Info and infolist hooks for buflist plugin */

#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "buflist.h"
#include "buflist-bar-item.h"


/*
 * Return buflist info "buflist_bar_items_count".
 */

char *
buflist_info_info_buflist_bar_items_count_cb (const void *pointer, void *data,
                                              const char *info_name,
                                              const char *arguments)
{
    char str_count[32];

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (str_count, sizeof (str_count), "%d", BUFLIST_BAR_NUM_ITEMS);

    return strdup (str_count);
}

/*
 * Add a buffer in an infolist.
 *
 * Return:
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
 * Return infolist "buflist".
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

    /* Make C compiler happy. */
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

    /* Build list with all buffers in buflist. */
    size = weechat_arraylist_size (buflist_list_buffers[item_index]);
    for (i = 0; i < size; i++)
    {
        ptr_buffer = weechat_arraylist_get (buflist_list_buffers[item_index], i);

        /* Check if ptr_buffer is still valid (buffer not closed). */
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
 * Hook info and infolist for buflist plugin.
 */

void
buflist_info_init (void)
{
    /* Info hooks */
    weechat_hook_info (
        "buflist_bar_items_count",
        N_("number of buflist bar items"),
        NULL,
        &buflist_info_info_buflist_bar_items_count_cb, NULL, NULL);

    /* Infolist hooks */
    weechat_hook_infolist (
        "buflist", N_("list of buffers in a buflist bar item"),
        NULL,
        N_("buflist bar item name (optional)"),
        &buflist_info_infolist_buflist_cb, NULL, NULL);
}
