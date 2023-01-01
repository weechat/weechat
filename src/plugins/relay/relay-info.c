/*
 * relay-info.c - info and infolist hooks for relay plugin
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include "relay-client.h"


/*
 * Returns relay info "relay_client_count".
 */

char *
relay_info_info_relay_client_count_cb (const void *pointer, void *data,
                                       const char *info_name,
                                       const char *arguments)
{
    char str_count[32], **items;
    const char *ptr_count;
    int count, protocol, status, num_items;
    struct t_relay_client *ptr_client;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    items = NULL;
    ptr_count = NULL;
    count = 0;
    protocol = -1;
    status = -1;

    items = weechat_string_split (arguments, ",", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_items);
    if (num_items > 2)
        goto end;

    if (num_items == 1)
    {
        /* one argument: try to guess if it's a protocol or a status */
        if (strcmp (items[0], "*") != 0)
        {
            protocol = relay_protocol_search (items[0]);
            if (protocol < 0)
            {
                status = relay_client_status_search (items[0]);
                if (status < 0)
                    goto end;
            }
        }
    }
    else if (num_items == 2)
    {
        /* two arguments: protocol,status */
        if (strcmp (items[0], "*") != 0)
        {
            protocol = relay_protocol_search (items[0]);
            if (protocol < 0)
                goto end;
        }
        if (strcmp (items[1], "*") != 0)
        {
            status = relay_client_status_search (items[1]);
            if (status < 0)
                goto end;
        }
    }

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if ((protocol >= 0) && ((int)ptr_client->protocol != protocol))
            continue;
        if ((status >= 0) && ((int)ptr_client->status != status))
            continue;
        count++;
    }

    snprintf (str_count, sizeof (str_count), "%d", count);
    ptr_count = str_count;

end:
    if (items)
        weechat_string_free_split (items);

    return (ptr_count) ? strdup (ptr_count) : NULL;
}

/*
 * Returns relay infolist "relay".
 */

struct t_infolist *
relay_info_infolist_relay_cb (const void *pointer, void *data,
                              const char *infolist_name,
                              void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_relay_client *ptr_client;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (obj_pointer && !relay_client_valid (obj_pointer))
        return NULL;

    ptr_infolist = weechat_infolist_new ();
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one relay */
        if (!relay_client_add_to_infolist (ptr_infolist, obj_pointer, 0))
        {
            weechat_infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all relays */
        for (ptr_client = relay_clients; ptr_client;
             ptr_client = ptr_client->next_client)
        {
            if (!relay_client_add_to_infolist (ptr_infolist, ptr_client, 0))
            {
                weechat_infolist_free (ptr_infolist);
                return NULL;
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Hooks infolist for relay plugin.
 */

void
relay_info_init ()
{
    /* info hooks */
    weechat_hook_info (
        "relay_client_count",
        N_("number of clients for relay"),
        /* TRANSLATORS: please do not translate the status names, they must be used in English */
        N_("protocol,status (both are optional, for each argument \"*\" "
           "means all; protocols: irc, weechat; statuses: connecting, "
           "waiting_auth, connected, auth_failed, disconnected)"),
        &relay_info_info_relay_client_count_cb, NULL, NULL);

    /* infolist hooks */
    weechat_hook_infolist (
        "relay", N_("list of relay clients"),
        N_("relay pointer (optional)"),
        NULL,
        &relay_info_infolist_relay_cb, NULL, NULL);
}
