/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * relay-info.c: info and infolist hooks for relay plugin
 */

#include <stdlib.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"


/*
 * Returns infolist with relay info.
 */

struct t_infolist *
relay_info_get_infolist_cb (void *data, const char *infolist_name,
                            void *pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_relay_client *ptr_client;

    /* make C compiler happy */
    (void) data;
    (void) arguments;

    if (!infolist_name || !infolist_name[0])
        return NULL;

    if (weechat_strcasecmp (infolist_name, "relay") == 0)
    {
        if (pointer && !relay_client_valid (pointer))
            return NULL;

        ptr_infolist = weechat_infolist_new ();
        if (ptr_infolist)
        {
            if (pointer)
            {
                /* build list with only one relay */
                if (!relay_client_add_to_infolist (ptr_infolist, pointer))
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
                    if (!relay_client_add_to_infolist (ptr_infolist, ptr_client))
                    {
                        weechat_infolist_free (ptr_infolist);
                        return NULL;
                    }
                }
                return ptr_infolist;
            }
        }
    }

    return NULL;
}

/*
 * Hooks infolist for relay plugin.
 */

void
relay_info_init ()
{
    weechat_hook_infolist ("relay", N_("list of relay clients"),
                           N_("relay pointer (optional)"),
                           NULL,
                           &relay_info_get_infolist_cb, NULL);
}
