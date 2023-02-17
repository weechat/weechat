/*
 * typing-bar-item.c - bar items for typing plugin
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "typing.h"
#include "typing-bar-item.h"
#include "typing-config.h"
#include "typing-status.h"


/*
 * Callback used to build a string with the list of nicks typing on the buffer.
 */

void
typing_bar_item_nicks_map_cb (void *data,
                              struct t_hashtable *hashtable,
                              const void *key, const void *value)
{
    char **str_nicks_typing;
    const char *ptr_nick;
    struct t_typing_status *ptr_typing_status;

    /* make C compiler happy */
    (void) hashtable;

    str_nicks_typing = (char **)data;

    ptr_nick = (const char *)key;
    ptr_typing_status = (struct t_typing_status *)value;

    if ((ptr_typing_status->state == TYPING_STATUS_STATE_TYPING)
        || (ptr_typing_status->state == TYPING_STATUS_STATE_PAUSED))
    {
        if (*str_nicks_typing[0])
            weechat_string_dyn_concat (str_nicks_typing, ", ", -1);
        if (ptr_typing_status->state == TYPING_STATUS_STATE_PAUSED)
            weechat_string_dyn_concat (str_nicks_typing, "(", -1);
        weechat_string_dyn_concat (str_nicks_typing, ptr_nick, -1);
        if (ptr_typing_status->state == TYPING_STATUS_STATE_PAUSED)
            weechat_string_dyn_concat (str_nicks_typing, ")", -1);
    }
}

/*
 * Returns content of bar item "typing": users currently typing on the buffer.
 */

char *
typing_bar_item_typing (const void *pointer, void *data,
                        struct t_gui_bar_item *item,
                        struct t_gui_window *window,
                        struct t_gui_buffer *buffer,
                        struct t_hashtable *extra_info)
{
    struct t_hashtable *ptr_nicks;
    char **str_nicks_typing, **str_typing, *str_typing_cut;
    int max_length;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!weechat_config_boolean (typing_config_look_enabled_nicks))
        return NULL;

    ptr_nicks = weechat_hashtable_get (typing_status_nicks, buffer);
    if (!ptr_nicks)
        return NULL;

    if (weechat_hashtable_get_integer (ptr_nicks, "items_count") == 0)
        return NULL;

    str_nicks_typing = weechat_string_dyn_alloc (128);
    weechat_hashtable_map (ptr_nicks,
                           &typing_bar_item_nicks_map_cb, str_nicks_typing);

    str_typing = weechat_string_dyn_alloc (256);
    /* TRANSLATORS: this text is displayed before the list of nicks typing in the bar item "typing", it must be as short as possible */
    weechat_string_dyn_concat (str_typing, _("Typing:"), -1);
    weechat_string_dyn_concat (str_typing, " ", -1);
    weechat_string_dyn_concat (str_typing, *str_nicks_typing, -1);

    weechat_string_dyn_free (str_nicks_typing, 1);

    max_length = weechat_config_integer (typing_config_look_item_max_length);
    if (max_length == 0)
        return weechat_string_dyn_free (str_typing, 0);

    str_typing_cut = weechat_string_cut (*str_typing, max_length, 1, 1, "…");

    weechat_string_dyn_free (str_typing, 1);

    return str_typing_cut;
}

/*
 * Initializes typing bar items.
 */

void
typing_bar_item_init ()
{
    weechat_bar_item_new (TYPING_BAR_ITEM_NAME,
                          &typing_bar_item_typing, NULL, NULL);
}
