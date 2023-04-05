/*
 * relay-completion.c - completion for relay command
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
#include "relay-server.h"


/*
 * Adds protocol and name to completion list.
 */

int
relay_completion_protocol_name_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_infolist *infolist;
    char protocol_name[1024];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) completion_item;

    infolist = weechat_infolist_get ("irc_server", NULL, NULL);
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            /* TCP socket */
            snprintf (protocol_name, sizeof (protocol_name), "irc.%s",
                      weechat_infolist_string (infolist, "name"));
            weechat_completion_list_add (completion, protocol_name,
                                         0, WEECHAT_LIST_POS_SORT);
            snprintf (protocol_name, sizeof (protocol_name), "tls.irc.%s",
                      weechat_infolist_string (infolist, "name"));
            weechat_completion_list_add (completion, protocol_name,
                                         0, WEECHAT_LIST_POS_SORT);
            /* UNIX domain socket */
            snprintf (protocol_name, sizeof (protocol_name), "unix.irc.%s",
                      weechat_infolist_string (infolist, "name"));
            weechat_completion_list_add (completion, protocol_name,
                                         0, WEECHAT_LIST_POS_SORT);
            snprintf (protocol_name, sizeof (protocol_name), "unix.tls.irc.%s",
                      weechat_infolist_string (infolist, "name"));
            weechat_completion_list_add (completion, protocol_name,
                                         0, WEECHAT_LIST_POS_SORT);
        }
        weechat_infolist_free (infolist);
    }

    /* TCP socket */
    weechat_completion_list_add (completion, "weechat",
                                 0, WEECHAT_LIST_POS_SORT);
    weechat_completion_list_add (completion, "tls.weechat",
                                 0, WEECHAT_LIST_POS_SORT);

    /* UNIX domain socket */
    weechat_completion_list_add (completion, "unix.weechat",
                                 0, WEECHAT_LIST_POS_SORT);
    weechat_completion_list_add (completion, "unix.tls.weechat",
                                 0, WEECHAT_LIST_POS_SORT);

    return WEECHAT_RC_OK;
}

/*
 * Adds protocol and name of current relays to completion list.
 */

int
relay_completion_relays_cb (const void *pointer, void *data,
                            const char *completion_item,
                            struct t_gui_buffer *buffer,
                            struct t_gui_completion *completion)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) completion_item;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_completion_list_add (completion,
                                     ptr_server->protocol_string,
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds free ports to completion list.
 */

int
relay_completion_free_port_cb (const void *pointer, void *data,
                               const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    struct t_relay_server *ptr_server;
    char str_port[16];
    int port_max;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) completion_item;

    port_max = -1;
    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->port > port_max)
            port_max = ptr_server->port;
    }
    if (port_max < 0)
        port_max = 8000 - 1;

    snprintf (str_port, sizeof (str_port), "%d", port_max + 1);
    weechat_completion_list_add (completion, str_port,
                                 0, WEECHAT_LIST_POS_SORT);

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
relay_completion_init ()
{
    weechat_hook_completion ("relay_protocol_name",
                             N_("all possible protocol.name for relay plugin"),
                             &relay_completion_protocol_name_cb, NULL, NULL);
    weechat_hook_completion ("relay_relays",
                             N_("protocol.name of current relays for relay "
                                "plugin"),
                             &relay_completion_relays_cb, NULL, NULL);
    weechat_hook_completion ("relay_free_port",
                             N_("first free port for relay plugin"),
                             &relay_completion_free_port_cb, NULL, NULL);
}
