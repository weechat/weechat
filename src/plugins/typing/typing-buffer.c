/*
 * typing-buffer.c - typing buffer list management
 *
 * Copyright (C) 2021 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>

#include "../weechat-plugin.h"
#include "typing.h"
#include "typing-buffer.h"


struct t_typing_buffer *typing_buffers = NULL;
struct t_typing_buffer *last_typing_buffer = NULL;


/*
 * Checks if a typing buffer pointer is valid.
 *
 * Returns:
 *   1: typing buffer exists
 *   0: typing buffer does not exist
 */

int
typing_buffer_valid (struct t_typing_buffer *typing_buffer)
{
    struct t_typing_buffer *ptr_typing_buffer;

    if (!typing_buffer)
        return 0;

    for (ptr_typing_buffer = typing_buffers; ptr_typing_buffer;
         ptr_typing_buffer = ptr_typing_buffer->next_buffer)
    {
        if (ptr_typing_buffer == typing_buffer)
            return 1;
    }

    /* typing_buffer not found */
    return 0;
}

/*
 * Adds a new buffer for typing status.
 *
 * Returns pointer to new typing buffer, NULL if error.
 */

struct t_typing_buffer *
typing_buffer_add (struct t_gui_buffer *buffer)
{
    struct t_typing_buffer *new_typing_buffer;

    if (!buffer)
        return NULL;

    if (weechat_typing_plugin->debug)
    {
        weechat_printf_date_tags (NULL, 0, "no_log",
                                  "%s: start typing for buffer \"%s\"",
                                  TYPING_PLUGIN_NAME,
                                  weechat_buffer_get_string (buffer, "name"));
    }

    new_typing_buffer = malloc (sizeof (*new_typing_buffer));
    if (new_typing_buffer)
    {
        new_typing_buffer->buffer = buffer;
        new_typing_buffer->status = TYPING_BUFFER_STATUS_OFF;
        new_typing_buffer->last_typed = 0;
        new_typing_buffer->last_signal_sent = 0;

        new_typing_buffer->prev_buffer = last_typing_buffer;
        new_typing_buffer->next_buffer = NULL;
        if (last_typing_buffer)
            last_typing_buffer->next_buffer = new_typing_buffer;
        else
            typing_buffers = new_typing_buffer;
        last_typing_buffer = new_typing_buffer;
    }

    return new_typing_buffer;
}

/*
 * Searches for typing buffer by buffer pointer.
 *
 * Returns pointer to typing buffer found, NULL if not found.
 */

struct t_typing_buffer *
typing_buffer_search_buffer (struct t_gui_buffer *buffer)
{
    struct t_typing_buffer *ptr_typing_buffer;

    for (ptr_typing_buffer = typing_buffers; ptr_typing_buffer;
         ptr_typing_buffer = ptr_typing_buffer->next_buffer)
    {
        if (ptr_typing_buffer->buffer == buffer)
            return ptr_typing_buffer;
    }

    /* typing buffer not found */
    return NULL;
}

/*
 * Removes a typing buffer from list.
 */

void
typing_buffer_free (struct t_typing_buffer *typing_buffer)
{
    struct t_typing_buffer *new_typing_buffers;
    struct t_gui_buffer *ptr_buffer;

    if (!typing_buffer)
        return;

    ptr_buffer = typing_buffer->buffer;

    /* remove typing buffer */
    if (last_typing_buffer == typing_buffer)
        last_typing_buffer = typing_buffer->prev_buffer;
    if (typing_buffer->prev_buffer)
    {
        (typing_buffer->prev_buffer)->next_buffer = typing_buffer->next_buffer;
        new_typing_buffers = typing_buffers;
    }
    else
        new_typing_buffers = typing_buffer->next_buffer;

    if (typing_buffer->next_buffer)
        (typing_buffer->next_buffer)->prev_buffer = typing_buffer->prev_buffer;

    free (typing_buffer);

    typing_buffers = new_typing_buffers;

    if (weechat_typing_plugin->debug)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            "%s: stop typing for buffer \"%s\"",
            TYPING_PLUGIN_NAME,
            weechat_buffer_get_string (ptr_buffer, "name"));
    }
}

/*
 * Adds a typing buffer in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
typing_buffer_add_to_infolist (struct t_infolist *infolist,
                               struct t_typing_buffer *typing_buffer)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !typing_buffer)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", typing_buffer->buffer))
        return 0;

    return 1;
}
