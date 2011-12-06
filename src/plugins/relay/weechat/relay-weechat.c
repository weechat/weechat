/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
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
 * relay-weechat.c: WeeChat protocol for relay to client
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>

#include "../../weechat-plugin.h"
#include "../relay.h"
#include "relay-weechat.h"
#include "relay-weechat-protocol.h"
#include "../relay-client.h"
#include "../relay-config.h"
#include "../relay-raw.h"


char *relay_weechat_compression_string[] = /* strings for compressions      */
{ "off", "gzip" };

char *relay_weechat_partial_message = NULL;


/*
 * relay_weechat_compression_search: search a compression by name
 */

int
relay_weechat_compression_search (const char *compression)
{
    int i;

    for (i = 0; i < RELAY_WEECHAT_NUM_COMPRESSIONS; i++)
    {
        if (weechat_strcasecmp (relay_weechat_compression_string[i], compression) == 0)
            return i;
    }

    /* compression not found */
    return -1;
}

/*
 * relay_weechat_recv: read data from client
 */

void
relay_weechat_recv (struct t_relay_client *client, const char *data)
{
    char *new_partial, *pos, *tmp, **commands;
    int num_commands, i;

    if (relay_weechat_partial_message)
    {
        new_partial = realloc (relay_weechat_partial_message,
                               strlen (relay_weechat_partial_message) +
                               strlen (data) + 1);
        if (!new_partial)
            return;
        relay_weechat_partial_message = new_partial;
        strcat (relay_weechat_partial_message, data);
    }
    else
        relay_weechat_partial_message = strdup (data);

    pos = strrchr (relay_weechat_partial_message, '\n');
    if (pos)
    {
        pos[0] = '\0';
        commands = weechat_string_split (relay_weechat_partial_message, "\n",
                                         0, 0, &num_commands);
        if (commands)
        {
            for (i = 0; i < num_commands; i++)
            {
                relay_weechat_protocol_recv (client, commands[i]);
            }
            weechat_string_free_split (commands);
        }
        if (pos[1])
        {
            tmp = strdup (pos + 1);
            free (relay_weechat_partial_message);
            relay_weechat_partial_message = tmp;
        }
        else
        {
            free (relay_weechat_partial_message);
            relay_weechat_partial_message = NULL;
        }
    }
}

/*
 * relay_weechat_close_connection: called when connection with client is closed
 */

void
relay_weechat_close_connection (struct t_relay_client *client)
{
    (void) client;
}

/*
 * relay_weechat_alloc: init relay data specific to weechat protocol
 */

void
relay_weechat_alloc (struct t_relay_client *client)
{
    struct t_relay_weechat_data *weechat_data;
    const char *password;

    password = weechat_config_string (relay_config_network_password);

    client->protocol_data = malloc (sizeof (*weechat_data));
    if (client->protocol_data)
    {
        RELAY_WEECHAT_DATA(client, password_ok) = (password && password[0]) ? 0 : 1;
#ifdef HAVE_ZLIB
        RELAY_WEECHAT_DATA(client, compression) = 1;
#else
        RELAY_WEECHAT_DATA(client, compression) = 0;
#endif
    }
}

/*
 * relay_weechat_alloc_with_infolist: init relay data specific to weechat
 *                                    protocol with an infolist
 */

void
relay_weechat_alloc_with_infolist (struct t_relay_client *client,
                                   struct t_infolist *infolist)
{
    struct t_relay_weechat_data *weechat_data;

    client->protocol_data = malloc (sizeof (*weechat_data));
    if (client->protocol_data)
    {
        RELAY_WEECHAT_DATA(client, password_ok) = weechat_infolist_integer (infolist, "password_ok");
        RELAY_WEECHAT_DATA(client, compression) = weechat_infolist_integer (infolist, "compression");
    }
}

/*
 * relay_weechat_free: free relay data specific to weechat protocol
 */

void
relay_weechat_free (struct t_relay_client *client)
{
    if (client->protocol_data)
        free (client->protocol_data);
}

/*
 * relay_weechat_add_to_infolist: add client weechat data in an infolist item
 *                                return 1 if ok, 0 if error
 */

int
relay_weechat_add_to_infolist (struct t_infolist_item *item,
                               struct t_relay_client *client)
{
    if (!item || !client)
        return 0;

    if (!weechat_infolist_new_var_integer (item, "password_ok", RELAY_WEECHAT_DATA(client, password_ok)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "compression", RELAY_WEECHAT_DATA(client, compression)))
        return 0;

    return 1;
}

/*
 * relay_weechat_print_log: print weechat client infos in log (usually for
 *                          crash dump)
 */

void
relay_weechat_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        weechat_log_printf ("    password_ok. . . . . . : %d",   RELAY_WEECHAT_DATA(client, password_ok));
        weechat_log_printf ("    compression. . . . . . : %d",   RELAY_WEECHAT_DATA(client, compression));
    }
}
