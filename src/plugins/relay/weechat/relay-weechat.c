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
 * relay_weechat_hook_signals: hook signals for a client
 */

void
relay_weechat_hook_signals (struct t_relay_client *client)
{
    RELAY_WEECHAT_DATA(client, hook_signal_buffer) =
        weechat_hook_signal ("buffer_*",
                             &relay_weechat_protocol_signal_buffer_cb,
                             client);
    RELAY_WEECHAT_DATA(client, hook_signal_nicklist) =
        weechat_hook_signal ("nicklist_*",
                             &relay_weechat_protocol_signal_nicklist_cb,
                             client);
    RELAY_WEECHAT_DATA(client, hook_signal_upgrade) =
        weechat_hook_signal ("upgrade*",
                             &relay_weechat_protocol_signal_upgrade_cb,
                             client);
}

/*
 * relay_weechat_unhook_signals: unhook signals for a client
 */

void
relay_weechat_unhook_signals (struct t_relay_client *client)
{
    if (RELAY_WEECHAT_DATA(client, hook_signal_buffer))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_buffer));
        RELAY_WEECHAT_DATA(client, hook_signal_buffer) = NULL;
    }
    if (RELAY_WEECHAT_DATA(client, hook_signal_nicklist))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_nicklist));
        RELAY_WEECHAT_DATA(client, hook_signal_nicklist) = NULL;
    }
    if (RELAY_WEECHAT_DATA(client, hook_signal_upgrade))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_upgrade));
        RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
    }
}

/*
 * relay_weechat_hook_timer_nicklist: timer to update nicklists for a client
 */

void
relay_weechat_hook_timer_nicklist (struct t_relay_client *client)
{
    RELAY_WEECHAT_DATA(client, hook_timer_nicklist) =
        weechat_hook_timer (100, 0, 1,
                            &relay_weechat_protocol_timer_nicklist_cb,
                            client);
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
    relay_weechat_unhook_signals (client);
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
        RELAY_WEECHAT_DATA(client, buffers_sync) =
            weechat_hashtable_new (16,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_INTEGER,
                                   NULL,
                                   NULL);
        RELAY_WEECHAT_DATA(client, hook_signal_buffer) = NULL;
        RELAY_WEECHAT_DATA(client, hook_signal_nicklist) = NULL;
        RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
        RELAY_WEECHAT_DATA(client, buffers_nicklist) =
            weechat_hashtable_new (16,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_STRING,
                                   NULL,
                                   NULL);
        RELAY_WEECHAT_DATA(client, hook_timer_nicklist) = NULL;
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
    int index, value;
    char name[64];
    const char *key, *str_value;

    client->protocol_data = malloc (sizeof (*weechat_data));
    if (client->protocol_data)
    {
        /* general stuff */
        RELAY_WEECHAT_DATA(client, password_ok) = weechat_infolist_integer (infolist, "password_ok");
        RELAY_WEECHAT_DATA(client, compression) = weechat_infolist_integer (infolist, "compression");

        /* sync of buffers */
        RELAY_WEECHAT_DATA(client, buffers_sync) = weechat_hashtable_new (16,
                                                                          WEECHAT_HASHTABLE_STRING,
                                                                          WEECHAT_HASHTABLE_INTEGER,
                                                                          NULL,
                                                                          NULL);
        index = 0;
        while (1)
        {
            snprintf (name, sizeof (name), "buffers_sync_name_%05d", index);
            key = weechat_infolist_string (infolist, name);
            if (!key)
                break;
            snprintf (name, sizeof (name), "buffers_sync_value_%05d", index);
            value = weechat_infolist_integer (infolist, name);
            weechat_hashtable_set (RELAY_WEECHAT_DATA(client, buffers_sync),
                                   key,
                                   &value);
            index++;
        }
        RELAY_WEECHAT_DATA(client, hook_signal_buffer) = NULL;
        RELAY_WEECHAT_DATA(client, hook_signal_nicklist) = NULL;
        RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
        RELAY_WEECHAT_DATA(client, buffers_nicklist) =
            weechat_hashtable_new (16,
                                   WEECHAT_HASHTABLE_STRING,
                                   WEECHAT_HASHTABLE_STRING,
                                   NULL,
                                   NULL);
        index = 0;
        while (1)
        {
            snprintf (name, sizeof (name), "buffers_nicklist_name_%05d", index);
            key = weechat_infolist_string (infolist, name);
            if (!key)
                break;
            snprintf (name, sizeof (name), "buffers_nicklist_value_%05d", index);
            str_value = weechat_infolist_string (infolist, name);
            weechat_hashtable_set (RELAY_WEECHAT_DATA(client, buffers_nicklist),
                                   key,
                                   str_value);
            index++;
        }
        RELAY_WEECHAT_DATA(client, hook_timer_nicklist) = NULL;

        if (weechat_hashtable_get_integer (RELAY_WEECHAT_DATA(client, buffers_sync),
                                           "items_count") > 0)
            relay_weechat_hook_signals (client);
        if (weechat_hashtable_get_integer (RELAY_WEECHAT_DATA(client, buffers_sync),
                                           "items_count") > 0)
            relay_weechat_hook_timer_nicklist (client);
    }
}

/*
 * relay_weechat_free: free relay data specific to weechat protocol
 */

void
relay_weechat_free (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        if (RELAY_WEECHAT_DATA(client, buffers_sync))
            weechat_hashtable_free (RELAY_WEECHAT_DATA(client, buffers_sync));
        if (RELAY_WEECHAT_DATA(client, hook_signal_buffer))
            weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_buffer));
        if (RELAY_WEECHAT_DATA(client, hook_signal_nicklist))
            weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_nicklist));
        if (RELAY_WEECHAT_DATA(client, hook_signal_upgrade))
            weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_upgrade));
        if (RELAY_WEECHAT_DATA(client, buffers_nicklist))
            weechat_hashtable_free (RELAY_WEECHAT_DATA(client, buffers_nicklist));

        free (client->protocol_data);

        client->protocol_data = NULL;
    }
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
    if (!weechat_hashtable_add_to_infolist (RELAY_WEECHAT_DATA(client, buffers_sync), item, "buffers_sync"))
        return 0;
    if (!weechat_hashtable_add_to_infolist (RELAY_WEECHAT_DATA(client, buffers_nicklist), item, "buffers_nicklist"))
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
        weechat_log_printf ("    buffers_sync . . . . . : 0x%lx (hashtable: '%s')",
                            RELAY_WEECHAT_DATA(client, buffers_sync),
                            weechat_hashtable_get_string (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                          "keys_values"));
        weechat_log_printf ("    hook_signal_buffer . . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_signal_buffer));
        weechat_log_printf ("    hook_signal_nicklist . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_signal_nicklist));
        weechat_log_printf ("    hook_signal_upgrade. . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_signal_upgrade));
        weechat_log_printf ("    buffers_nicklist . . . : 0x%lx (hashtable: '%s')",
                            RELAY_WEECHAT_DATA(client, buffers_nicklist),
                            weechat_hashtable_get_string (RELAY_WEECHAT_DATA(client, buffers_nicklist),
                                                          "keys_values"));
        weechat_log_printf ("    hook_timer_nicklist. . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_timer_nicklist));
    }
}
