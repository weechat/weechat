/*
 * relay-bar-item.c - bar items for relay plugin
 *
 * Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "relay.h"
#include "relay-config.h"
#include "relay-remote.h"


/*
 * Returns content of bar item "input_prompt".
 */

char *
relay_bar_item_input_prompt (const void *pointer, void *data,
                             struct t_gui_bar_item *item,
                             struct t_gui_window *window,
                             struct t_gui_buffer *buffer,
                             struct t_hashtable *extra_info)
{
    char str_status[512], *input_prompt;
    int fetching_data;
    const char *ptr_input_prompt;
    struct t_relay_remote *ptr_remote;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) extra_info;

    if (!buffer)
        return NULL;

    str_status[0] = '\0';
    ptr_remote = relay_remote_search (weechat_buffer_get_string (buffer, "localvar_relay_remote"));
    if (ptr_remote)
    {
        fetching_data = (ptr_remote->status == RELAY_STATUS_CONNECTED)
            && !ptr_remote->synced;
        if ((ptr_remote->status != RELAY_STATUS_CONNECTED) || fetching_data)
        {
            snprintf (
                str_status, sizeof (str_status),
                "%s<%s%s%s%s>",
                weechat_color (weechat_config_string (relay_config_color_status[ptr_remote->status])),
                _(relay_status_string[ptr_remote->status]),
                (fetching_data) ? " (" : "",
                (fetching_data) ? _("fetching data") : "",
                (fetching_data) ? ")" : "");
        }
    }

    ptr_input_prompt = weechat_buffer_get_string (buffer, "input_prompt");
    if (!ptr_input_prompt && !str_status[0])
        return NULL;

    if (weechat_asprintf (
            &input_prompt, "%s%s%s",
            (ptr_input_prompt) ? ptr_input_prompt : "",
            (ptr_input_prompt && ptr_input_prompt[0] && str_status[0]) ? " " : "",
            str_status) >= 0)
        return input_prompt;

    return NULL;
}

/*
 * Initializes relay bar items.
 */

void
relay_bar_item_init (void)
{
    weechat_bar_item_new ("input_prompt",
                          &relay_bar_item_input_prompt, NULL, NULL);
}
