/*
 * relay-api.c - API protocol for relay to client
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "../relay-client.h"
#include "../relay-config.h"
#include "../relay-http.h"
#include "../relay-raw.h"
#include "../relay-raw.h"
#include "relay-api.h"
#include "relay-api-protocol.h"


/*
 * Returns buffer id.
 */

long long
relay_api_get_buffer_id (struct t_gui_buffer *buffer)
{
    const char *ptr_id;
    char *error;
    long long id;

    if (!buffer)
        return -1;

    ptr_id = weechat_buffer_get_string (buffer, "id");
    if (!ptr_id)
        return -1;

    error = NULL;
    id = strtoll (ptr_id, &error, 10);
    if (!error || error[0])
        return -1;

    return id;
}

/*
 * Returns value of "colors" URL parameter, an enum with one of these values:
 *   - RELAY_API_COLORS_ANSI (default)
 *   - RELAY_API_COLORS_WEECHAT
 *   - RELAY_API_COLORS_STRIP
 */

enum t_relay_api_colors
relay_api_search_colors (const char *colors)
{
    if (!colors)
        return RELAY_API_COLORS_ANSI;

    if (strcmp (colors, "weechat") == 0)
        return RELAY_API_COLORS_WEECHAT;

    if (strcmp (colors, "strip") == 0)
        return RELAY_API_COLORS_STRIP;

    return RELAY_API_COLORS_ANSI;
}

/*
 * Hooks signals for a client.
 */

void
relay_api_hook_signals (struct t_relay_client *client)
{
    if (!RELAY_API_DATA(client, hook_signal_buffer))
    {
        RELAY_API_DATA(client, hook_signal_buffer) =
            weechat_hook_signal ("buffer_*",
                                 &relay_api_protocol_signal_buffer_cb,
                                 client, NULL);
    }
    if (RELAY_API_DATA(client, sync_nicks))
    {
        if (!RELAY_API_DATA(client, hook_hsignal_nicklist))
        {
            RELAY_API_DATA(client, hook_hsignal_nicklist) =
                weechat_hook_hsignal ("nicklist_*",
                                      &relay_api_protocol_hsignal_nicklist_cb,
                                      client, NULL);
        }
    }
    else
    {
        if (RELAY_API_DATA(client, hook_hsignal_nicklist))
        {
            weechat_unhook (RELAY_API_DATA(client, hook_hsignal_nicklist));
            RELAY_API_DATA(client, hook_hsignal_nicklist) = NULL;
        }
    }
    if (RELAY_API_DATA(client, sync_input))
    {
        if (!RELAY_API_DATA(client, hook_signal_input))
        {
            RELAY_API_DATA(client, hook_signal_input) =
                weechat_hook_signal ("input_prompt_changed;"
                                     "input_text_changed;"
                                     "input_text_cursor_moved",
                                     &relay_api_protocol_signal_input_cb,
                                     client, NULL);
        }
    }
    else
    {
        if (RELAY_API_DATA(client, hook_signal_input))
        {
            weechat_unhook (RELAY_API_DATA(client, hook_signal_input));
            RELAY_API_DATA(client, hook_signal_input) = NULL;
        }
    }
    if (!RELAY_API_DATA(client, hook_signal_upgrade))
    {
        RELAY_API_DATA(client, hook_signal_upgrade) =
            weechat_hook_signal ("upgrade*;quit",
                                 &relay_api_protocol_signal_upgrade_cb,
                                 client, NULL);
    }
}

/*
 * Unhooks signals for a client.
 */

void
relay_api_unhook_signals (struct t_relay_client *client)
{
    if (RELAY_API_DATA(client, hook_signal_buffer))
    {
        weechat_unhook (RELAY_API_DATA(client, hook_signal_buffer));
        RELAY_API_DATA(client, hook_signal_buffer) = NULL;
    }
    if (RELAY_API_DATA(client, hook_hsignal_nicklist))
    {
        weechat_unhook (RELAY_API_DATA(client, hook_hsignal_nicklist));
        RELAY_API_DATA(client, hook_hsignal_nicklist) = NULL;
    }
    if (RELAY_API_DATA(client, hook_signal_input))
    {
        weechat_unhook (RELAY_API_DATA(client, hook_signal_input));
        RELAY_API_DATA(client, hook_signal_input) = NULL;
    }
    if (RELAY_API_DATA(client, hook_signal_upgrade))
    {
        weechat_unhook (RELAY_API_DATA(client, hook_signal_upgrade));
        RELAY_API_DATA(client, hook_signal_upgrade) = NULL;
    }
}

/*
 * Reads HTTP request from a client.
 */

void
relay_api_recv_http (struct t_relay_client *client)
{
    relay_api_protocol_recv_http (client);
}

/*
 * Reads JSON string from a client.
 */

void
relay_api_recv_json (struct t_relay_client *client, const char *json)
{
    relay_api_protocol_recv_json (client, json);
}

/*
 * Closes connection with a client.
 */

void
relay_api_close_connection (struct t_relay_client *client)
{
    /*
     * IMPORTANT: if changes are made in this function or sub-functions called,
     * please also update the function relay_api_add_to_infolist:
     * when the flag force_disconnected_state is set to 1 we simulate
     * a disconnected state for client in infolist (used on /upgrade -save)
     */

    relay_api_unhook_signals (client);
}

/*
 * Initializes relay data specific to API protocol.
 */

void
relay_api_alloc (struct t_relay_client *client)
{
    client->protocol_data = malloc (sizeof (struct t_relay_api_data));
    if (!client->protocol_data)
        return;

    RELAY_API_DATA(client, hook_signal_buffer) = NULL;
    RELAY_API_DATA(client, hook_hsignal_nicklist) = NULL;
    RELAY_API_DATA(client, hook_signal_input) = NULL;
    RELAY_API_DATA(client, hook_signal_upgrade) = NULL;
    RELAY_API_DATA(client, buffers_closing) = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_POINTER,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    RELAY_API_DATA(client, sync_enabled) = 0;
    RELAY_API_DATA(client, sync_nicks) = 0;
    RELAY_API_DATA(client, sync_input) = 0;
    RELAY_API_DATA(client, sync_colors) = RELAY_API_COLORS_ANSI;
}

/*
 * Initializes relay data specific to API protocol with an infolist.
 *
 * This is called after /upgrade.
 */

void
relay_api_alloc_with_infolist (struct t_relay_client *client,
                               struct t_infolist *infolist)
{
    client->protocol_data = malloc (sizeof (struct t_relay_api_data));
    if (!client->protocol_data)
        return;

    RELAY_API_DATA(client, hook_signal_buffer) = NULL;
    RELAY_API_DATA(client, hook_hsignal_nicklist) = NULL;
    RELAY_API_DATA(client, hook_signal_input) = NULL;
    RELAY_API_DATA(client, hook_signal_upgrade) = NULL;
    RELAY_API_DATA(client, buffers_closing) = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_POINTER,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    RELAY_API_DATA(client, sync_enabled) = weechat_infolist_integer (
        infolist, "sync_enabled");
    RELAY_API_DATA(client, sync_nicks) = weechat_infolist_integer (
        infolist, "sync_nicks");
    RELAY_API_DATA(client, sync_input) = weechat_infolist_integer (
        infolist, "sync_input");
    RELAY_API_DATA(client, sync_colors) = weechat_infolist_integer (
        infolist, "sync_colors");

    if (!RELAY_STATUS_HAS_ENDED(client->status)
        && RELAY_API_DATA(client, sync_enabled))
    {
        relay_api_hook_signals (client);
    }
}

/*
 * Returns the client initial status: it is always "authenticating" for API
 * protocol because we always expect the client to authenticate.
 */

enum t_relay_status
relay_api_get_initial_status (struct t_relay_client *client)
{
    /* make C compiler happy */
    (void) client;

    return RELAY_STATUS_AUTHENTICATING;
}

/*
 * Frees relay data specific to API protocol.
 */

void
relay_api_free (struct t_relay_client *client)
{
    if (!client)
        return;

    if (client->protocol_data)
    {
        weechat_unhook (RELAY_API_DATA(client, hook_signal_buffer));
        weechat_unhook (RELAY_API_DATA(client, hook_hsignal_nicklist));
        weechat_unhook (RELAY_API_DATA(client, hook_signal_input));
        weechat_unhook (RELAY_API_DATA(client, hook_signal_upgrade));
        weechat_hashtable_free (RELAY_API_DATA(client, buffers_closing));

        free (client->protocol_data);

        client->protocol_data = NULL;
    }
}

/*
 * Adds client API data in an infolist.
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
relay_api_add_to_infolist (struct t_infolist_item *item,
                           struct t_relay_client *client,
                           int force_disconnected_state)
{
    if (!item || !client)
        return 0;

    /* parameter not used today, it may be in future */
    (void) force_disconnected_state;

    if (!weechat_infolist_new_var_pointer (item, "hook_signal_buffer", RELAY_API_DATA(client, hook_signal_buffer)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_hsignal_nicklist", RELAY_API_DATA(client, hook_hsignal_nicklist)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_input", RELAY_API_DATA(client, hook_signal_input)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_upgrade", RELAY_API_DATA(client, hook_signal_upgrade)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_enabled", RELAY_API_DATA(client, sync_enabled)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_nicks", RELAY_API_DATA(client, sync_nicks)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_input", RELAY_API_DATA(client, sync_input)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_colors", RELAY_API_DATA(client, sync_colors)))
        return 0;

    return 1;
}

/*
 * Prints client API data in WeeChat log file (usually for crash dump).
 */

void
relay_api_print_log (struct t_relay_client *client)
{
    if (client->protocol_data)
    {
        weechat_log_printf ("    hook_signal_buffer. . . : %p", RELAY_API_DATA(client, hook_signal_buffer));
        weechat_log_printf ("    hook_hsignal_nicklist . : %p", RELAY_API_DATA(client, hook_hsignal_nicklist));
        weechat_log_printf ("    hook_signal_input . . . : %p", RELAY_API_DATA(client, hook_signal_input));
        weechat_log_printf ("    hook_signal_upgrade . . : %p", RELAY_API_DATA(client, hook_signal_upgrade));
        weechat_log_printf ("    buffers_closing. . . . .: %p (hashtable: '%s')",
                            RELAY_API_DATA(client, buffers_closing),
                            weechat_hashtable_get_string (
                                RELAY_API_DATA(client, buffers_closing),
                                "keys_values"));
        weechat_log_printf ("    sync_enabled. . . . . . : %d", RELAY_API_DATA(client, sync_enabled));
        weechat_log_printf ("    sync_nicks. . . . . . . : %d", RELAY_API_DATA(client, sync_nicks));
        weechat_log_printf ("    sync_input. . . . . . . : %d", RELAY_API_DATA(client, sync_input));
        weechat_log_printf ("    sync_colors . . . . . . : %d", RELAY_API_DATA(client, sync_colors));
    }
}
