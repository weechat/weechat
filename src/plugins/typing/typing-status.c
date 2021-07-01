/*
 * typing-status.c - manage self and other users typing status
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
#include "typing-status.h"


struct t_hashtable *typing_status_self = NULL;


/*
 * Removes a typing status.
 */

void
typing_status_free_value_cb (struct t_hashtable *hashtable,
                             const void *key, const void *value)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) hashtable;

    ptr_buffer = (struct t_gui_buffer *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if (!ptr_typing_status)
        return;

    if (weechat_typing_plugin->debug)
    {
        weechat_printf_date_tags (
            NULL, 0, "no_log",
            "%s: stop typing status for buffer \"%s\"",
            TYPING_PLUGIN_NAME,
            weechat_buffer_get_string (ptr_buffer, "name"));
    }

    free (ptr_typing_status);
}

/*
 * Adds a new typing status.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

struct t_typing_status *
typing_status_add (struct t_gui_buffer *buffer)
{
    struct t_typing_status *new_typing_status;

    if (!buffer)
        return NULL;

    if (!typing_status_self)
    {
        typing_status_self = weechat_hashtable_new (64,
                                                    WEECHAT_HASHTABLE_POINTER,
                                                    WEECHAT_HASHTABLE_POINTER,
                                                    NULL,
                                                    NULL);
        if (!typing_status_self)
            return NULL;
        weechat_hashtable_set_pointer (typing_status_self,
                                       "callback_free_value",
                                       &typing_status_free_value_cb);
    }

    new_typing_status = malloc (sizeof (*new_typing_status));
    if (!new_typing_status)
        return NULL;

    if (weechat_typing_plugin->debug)
    {
        weechat_printf_date_tags (NULL, 0, "no_log",
                                  "%s: start typing status for buffer \"%s\"",
                                  TYPING_PLUGIN_NAME,
                                  weechat_buffer_get_string (buffer, "name"));
    }

    new_typing_status->status = TYPING_STATUS_STATUS_OFF;
    new_typing_status->last_typed = 0;
    new_typing_status->last_signal_sent = 0;

    weechat_hashtable_set (typing_status_self, buffer, new_typing_status);

    return new_typing_status;
}

/*
 * Ends typing status.
 */

void
typing_status_end ()
{
    if (typing_status_self)
    {
        weechat_hashtable_free (typing_status_self);
        typing_status_self = NULL;
    }
}
