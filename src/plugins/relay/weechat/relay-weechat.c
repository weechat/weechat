/*
 * relay-weechat.c - WeeChat protocol for relay to client
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
#include "relay-weechat-nicklist.h"
#include "relay-weechat-protocol.h"
#include "../relay-client.h"
#include "../relay-config.h"
#include "../relay-raw.h"


char *relay_weechat_compression_string[] = /* strings for compression       */
{ "off", "zlib", "zstd" };


/*
 * Searches for a compression.
 *
 * Returns index of compression in enum t_relay_weechat_compression, -1 if
 * compression is not found.
 */

int
relay_weechat_compression_search (const char *compression)
{
    int i;

    if (!compression)
        return -1;

    for (i = 0; i < RELAY_WEECHAT_NUM_COMPRESSIONS; i++)
    {
        if (strcmp (relay_weechat_compression_string[i], compression) == 0)
            return i;
    }

    /* compression not found */
    return -1;
}

/*
 * Hooks signals for a client.
 */

void
relay_weechat_hook_signals (struct t_relay_client *client)
{
    RELAY_WEECHAT_DATA(client, hook_signal_buffer) =
        weechat_hook_signal ("buffer_*",
                             &relay_weechat_protocol_signal_buffer_cb,
                             client, NULL);
    RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist) =
        weechat_hook_hsignal ("nicklist_*",
                              &relay_weechat_protocol_hsignal_nicklist_cb,
                              client, NULL);
    RELAY_WEECHAT_DATA(client, hook_signal_upgrade) =
        weechat_hook_signal ("upgrade*",
                             &relay_weechat_protocol_signal_upgrade_cb,
                             client, NULL);
}

/*
 * Unhooks signals for a client.
 */

void
relay_weechat_unhook_signals (struct t_relay_client *client)
{
    if (RELAY_WEECHAT_DATA(client, hook_signal_buffer))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_buffer));
        RELAY_WEECHAT_DATA(client, hook_signal_buffer) = NULL;
    }
    if (RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist));
        RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
    }
    if (RELAY_WEECHAT_DATA(client, hook_signal_upgrade))
    {
        weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_upgrade));
        RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
    }
}

/*
 * Hooks timer to update nicklist.
 */

void
relay_weechat_hook_timer_nicklist (struct t_relay_client *client)
{
    RELAY_WEECHAT_DATA(client, hook_timer_nicklist) =
        weechat_hook_timer (100, 0, 1,
                            &relay_weechat_protocol_timer_nicklist_cb,
                            client, NULL);
}

/*
 * Reads data from a client.
 */

void
relay_weechat_recv (struct t_relay_client *client, const char *data)
{
    relay_weechat_protocol_recv (client, data);
}

/*
 * Closes connection with a client.
 */

void
relay_weechat_close_connection (struct t_relay_client *client)
{
    /*
     * IMPORTANT: if changes are made in this function or sub-functions called,
     * please also update the function relay_weechat_add_to_infolist:
     * when the flag force_disconnected_state is set to 1 we simulate
     * a disconnected state for client in infolist (used on /upgrade -save)
     */

    relay_weechat_unhook_signals (client);
}

/*
 * Frees a value of hashtable "buffers_nicklist".
 */

void
relay_weechat_free_buffers_nicklist (struct t_hashtable *hashtable,
                                     const void *key, void *value)
{
    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    relay_weechat_nicklist_free ((struct t_relay_weechat_nicklist *)value);
}

/*
 * Initializes relay data specific to WeeChat protocol.
 */

void
relay_weechat_alloc (struct t_relay_client *client)
{
    client->protocol_data = malloc (sizeof (struct t_relay_weechat_data));
    if (!client->protocol_data)
        return;

    RELAY_WEECHAT_DATA(client, handshake_done) = 0;
    RELAY_WEECHAT_DATA(client, password_ok) = 0;
    RELAY_WEECHAT_DATA(client, totp_ok) = 0;
    RELAY_WEECHAT_DATA(client, compression) = RELAY_WEECHAT_COMPRESSION_OFF;
    RELAY_WEECHAT_DATA(client, escape_commands) = 0;
    RELAY_WEECHAT_DATA(client, buffers_sync) =
        weechat_hashtable_new (32,
                               WEECHAT_HASHTABLE_STRING,
                               WEECHAT_HASHTABLE_INTEGER,
                               NULL, NULL);
    RELAY_WEECHAT_DATA(client, hook_signal_buffer) = NULL;
    RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
    RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
    RELAY_WEECHAT_DATA(client, buffers_nicklist) =
        weechat_hashtable_new (32,
                               WEECHAT_HASHTABLE_POINTER,
                               WEECHAT_HASHTABLE_POINTER,
                               NULL, NULL);
    weechat_hashtable_set_pointer (RELAY_WEECHAT_DATA(client, buffers_nicklist),
                                   "callback_free_value",
                                   &relay_weechat_free_buffers_nicklist);
    RELAY_WEECHAT_DATA(client, hook_timer_nicklist) = NULL;

    relay_weechat_hook_signals (client);
}

/*
 * Initializes relay data specific to WeeChat protocol with an infolist.
 *
 * This is called after /upgrade.
 */

void
relay_weechat_alloc_with_infolist (struct t_relay_client *client,
                                   struct t_infolist *infolist)
{
    int index, value;
    char name[64];
    const char *key;

    client->protocol_data = malloc (sizeof (struct t_relay_weechat_data));
    if (client->protocol_data)
    {
        /* general stuff */
        /* "handshake_done" is new in WeeChat 2.9 */
        if (weechat_infolist_search_var (infolist, "handshake_done"))
            RELAY_WEECHAT_DATA(client, handshake_done) = weechat_infolist_integer (infolist, "handshake_done");
        else
            RELAY_WEECHAT_DATA(client, handshake_done) = 0;
        RELAY_WEECHAT_DATA(client, password_ok) = weechat_infolist_integer (
            infolist, "password_ok");
        /* "totp_ok" is new in WeeChat 2.4 */
        if (weechat_infolist_search_var (infolist, "totp_ok"))
            RELAY_WEECHAT_DATA(client, totp_ok) = weechat_infolist_integer (infolist, "totp_ok");
        else
            RELAY_WEECHAT_DATA(client, totp_ok) = 1;
        RELAY_WEECHAT_DATA(client, compression) = weechat_infolist_integer (
            infolist, "compression");
        RELAY_WEECHAT_DATA(client, escape_commands) = weechat_infolist_integer (
            infolist, "escape_commands");

        /* sync of buffers */
        RELAY_WEECHAT_DATA(client, buffers_sync) = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_INTEGER,
            NULL, NULL);
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
        RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
        RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
        RELAY_WEECHAT_DATA(client, buffers_nicklist) =
            weechat_hashtable_new (32,
                                   WEECHAT_HASHTABLE_POINTER,
                                   WEECHAT_HASHTABLE_POINTER,
                                   NULL, NULL);
        weechat_hashtable_set_pointer (RELAY_WEECHAT_DATA(client, buffers_nicklist),
                                       "callback_free_value",
                                       &relay_weechat_free_buffers_nicklist);
        RELAY_WEECHAT_DATA(client, hook_timer_nicklist) = NULL;

        if (RELAY_CLIENT_HAS_ENDED(client))
        {
            RELAY_WEECHAT_DATA(client, hook_signal_buffer) = NULL;
            RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist) = NULL;
            RELAY_WEECHAT_DATA(client, hook_signal_upgrade) = NULL;
        }
        else
            relay_weechat_hook_signals (client);
    }
}

/*
 * Returns the client initial status: it is always  "waiting_auth" for weechat
 * protocol because we always expect the "init" command, even without any
 * password.
 */

enum t_relay_status
relay_weechat_get_initial_status (struct t_relay_client *client)
{
    /* make C compiler happy */
    (void) client;

    return RELAY_STATUS_WAITING_AUTH;
}

/*
 * Frees relay data specific to WeeChat protocol.
 */

void
relay_weechat_free (struct t_relay_client *client)
{
    if (!client)
        return;

    if (client->protocol_data)
    {
        if (RELAY_WEECHAT_DATA(client, buffers_sync))
            weechat_hashtable_free (RELAY_WEECHAT_DATA(client, buffers_sync));
        if (RELAY_WEECHAT_DATA(client, hook_signal_buffer))
            weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_buffer));
        if (RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist))
            weechat_unhook (RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist));
        if (RELAY_WEECHAT_DATA(client, hook_signal_upgrade))
            weechat_unhook (RELAY_WEECHAT_DATA(client, hook_signal_upgrade));
        if (RELAY_WEECHAT_DATA(client, buffers_nicklist))
            weechat_hashtable_free (RELAY_WEECHAT_DATA(client, buffers_nicklist));

        free (client->protocol_data);

        client->protocol_data = NULL;
    }
}

/*
 * Adds client WeeChat data in an infolist.
 *
 * If force_disconnected_state == 1, the infolist contains the client
 * in a disconnected state (but the client is unchanged, still connected if it
 * was).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_weechat_add_to_infolist (struct t_infolist_item *item,
                               struct t_relay_client *client,
                               int force_disconnected_state)
{
    if (!item || !client)
        return 0;

    /* parameter not used today, it may be in future */
    (void) force_disconnected_state;

    if (!weechat_infolist_new_var_integer (item, "handshake_done", RELAY_WEECHAT_DATA(client, handshake_done)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "password_ok", RELAY_WEECHAT_DATA(client, password_ok)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "totp_ok", RELAY_WEECHAT_DATA(client, totp_ok)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "compression", RELAY_WEECHAT_DATA(client, compression)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "escape_commands", RELAY_WEECHAT_DATA(client, escape_commands)))
        return 0;
    if (!weechat_hashtable_add_to_infolist (RELAY_WEECHAT_DATA(client, buffers_sync), item, "buffers_sync"))
        return 0;

    return 1;
}

/*
 * Prints client WeeChat data in WeeChat log file (usually for crash dump).
 */

void
relay_weechat_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        weechat_log_printf ("    handshake_done. . . . . : %d",   RELAY_WEECHAT_DATA(client, handshake_done));
        weechat_log_printf ("    password_ok . . . . . . : %d",   RELAY_WEECHAT_DATA(client, password_ok));
        weechat_log_printf ("    totp_ok . . . . . . . . : %d",   RELAY_WEECHAT_DATA(client, totp_ok));
        weechat_log_printf ("    compression . . . . . . : %d",   RELAY_WEECHAT_DATA(client, compression));
        weechat_log_printf ("    escape_commands . . . . : %d",   RELAY_WEECHAT_DATA(client, escape_commands));
        weechat_log_printf ("    buffers_sync. . . . . . : 0x%lx (hashtable: '%s')",
                            RELAY_WEECHAT_DATA(client, buffers_sync),
                            weechat_hashtable_get_string (RELAY_WEECHAT_DATA(client, buffers_sync),
                                                          "keys_values"));
        weechat_log_printf ("    hook_signal_buffer. . . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_signal_buffer));
        weechat_log_printf ("    hook_hsignal_nicklist . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_hsignal_nicklist));
        weechat_log_printf ("    hook_signal_upgrade . . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_signal_upgrade));
        weechat_log_printf ("    buffers_nicklist. . . . : 0x%lx (hashtable: '%s')",
                            RELAY_WEECHAT_DATA(client, buffers_nicklist),
                            weechat_hashtable_get_string (RELAY_WEECHAT_DATA(client, buffers_nicklist),
                                                          "keys_values"));
        weechat_log_printf ("    hook_timer_nicklist . . : 0x%lx", RELAY_WEECHAT_DATA(client, hook_timer_nicklist));
    }
}
