/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* API protocol for relay to client */

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
 * Return value of "colors" URL parameter, an enum with one of these values:
 *   - RELAY_API_COLORS_ANSI (default)
 *   - RELAY_API_COLORS_WEECHAT
 *   - RELAY_API_COLORS_STRIP
 */

int
relay_api_search_colors (const char *colors)
{
    if (!colors)
        return -1;

    if (strcmp (colors, "ansi") == 0)
        return RELAY_API_COLORS_ANSI;
    if (strcmp (colors, "weechat") == 0)
        return RELAY_API_COLORS_WEECHAT;
    if (strcmp (colors, "strip") == 0)
        return RELAY_API_COLORS_STRIP;

    return -1;
}

/*
 * Add an option (mask with "*" allowed) to synchronize for a client.
 *
 * Empty strings, strings with a newline and duplicates are ignored.
 *
 * Note: the hooks on options are not changed: relay_api_unhook_signals and
 * relay_api_hook_signals must be called after this function.
 */

void
relay_api_add_sync_option (struct t_relay_client *client, const char *option)
{
    char **new_options;
    int i;

    if (!client || !option || !option[0] || strchr (option, '\n'))
        return;

    for (i = 0; i < RELAY_API_DATA(client, num_sync_options); i++)
    {
        if (strcmp (RELAY_API_DATA(client, sync_options)[i], option) == 0)
            return;
    }

    new_options = realloc (
        RELAY_API_DATA(client, sync_options),
        (RELAY_API_DATA(client, num_sync_options) + 2) * sizeof (*new_options));
    if (!new_options)
        return;
    RELAY_API_DATA(client, sync_options) = new_options;

    new_options[RELAY_API_DATA(client, num_sync_options)] = strdup (option);
    if (!new_options[RELAY_API_DATA(client, num_sync_options)])
        return;
    RELAY_API_DATA(client, num_sync_options)++;
    new_options[RELAY_API_DATA(client, num_sync_options)] = NULL;
}

/*
 * Free options to synchronize for a client.
 *
 * Note: the hooks on options are not changed: relay_api_unhook_signals must
 * be called before this function.
 */

void
relay_api_free_sync_options (struct t_relay_client *client)
{
    if (!client)
        return;

    weechat_string_free_split (RELAY_API_DATA(client, sync_options));
    RELAY_API_DATA(client, sync_options) = NULL;
    RELAY_API_DATA(client, num_sync_options) = 0;
}

/*
 * Unhook config options for a client.
 */

void
relay_api_unhook_config_options (struct t_relay_client *client)
{
    int i;

    if (!client || !RELAY_API_DATA(client, hook_config_options))
        return;

    for (i = 0; i < RELAY_API_DATA(client, num_sync_options); i++)
    {
        weechat_unhook (RELAY_API_DATA(client, hook_config_options)[i]);
    }
    free (RELAY_API_DATA(client, hook_config_options));
    RELAY_API_DATA(client, hook_config_options) = NULL;
}

/*
 * Hook signals for a client.
 *
 * If "sync_notify_only" is set, only signals "buffer_line_added", "upgrade*"
 * and "quit" are hooked (options in "sync_options" are not hooked).
 *
 * Note: the hook on signals "buffer_*" is not changed if it already exists,
 * so relay_api_unhook_signals must be called before this function when
 * "sync_notify_only" is changed.
 */

void
relay_api_hook_signals (struct t_relay_client *client)
{
    int i;

    if (!RELAY_API_DATA(client, hook_signal_buffer))
    {
        RELAY_API_DATA(client, hook_signal_buffer) =
            weechat_hook_signal ((RELAY_API_DATA(client, sync_notify_only)) ?
                                 "buffer_line_added" : "buffer_*",
                                 &relay_api_protocol_signal_buffer_cb,
                                 client, NULL);
    }
    if (RELAY_API_DATA(client, sync_nicks)
        && !RELAY_API_DATA(client, sync_notify_only))
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
    if (RELAY_API_DATA(client, sync_input)
        && !RELAY_API_DATA(client, sync_notify_only))
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
    if (!RELAY_API_DATA(client, sync_notify_only))
    {
        if (!RELAY_API_DATA(client, hook_signal_day_changed))
        {
            RELAY_API_DATA(client, hook_signal_day_changed) =
                weechat_hook_signal ("day_changed",
                                     &relay_api_protocol_signal_day_changed_cb,
                                     client, NULL);
        }
    }
    else
    {
        if (RELAY_API_DATA(client, hook_signal_day_changed))
        {
            weechat_unhook (RELAY_API_DATA(client, hook_signal_day_changed));
            RELAY_API_DATA(client, hook_signal_day_changed) = NULL;
        }
    }
    if (!RELAY_API_DATA(client, sync_notify_only)
        && (RELAY_API_DATA(client, num_sync_options) > 0))
    {
        if (!RELAY_API_DATA(client, hook_config_options))
        {
            RELAY_API_DATA(client, hook_config_options) = calloc (
                RELAY_API_DATA(client, num_sync_options),
                sizeof (*RELAY_API_DATA(client, hook_config_options)));
            if (RELAY_API_DATA(client, hook_config_options))
            {
                for (i = 0; i < RELAY_API_DATA(client, num_sync_options); i++)
                {
                    RELAY_API_DATA(client, hook_config_options)[i] =
                        weechat_hook_config (
                            RELAY_API_DATA(client, sync_options)[i],
                            &relay_api_protocol_config_option_cb,
                            client, NULL);
                }
            }
        }
    }
    else
    {
        relay_api_unhook_config_options (client);
    }
}

/*
 * Unhook signals for a client.
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
    if (RELAY_API_DATA(client, hook_signal_day_changed))
    {
        weechat_unhook (RELAY_API_DATA(client, hook_signal_day_changed));
        RELAY_API_DATA(client, hook_signal_day_changed) = NULL;
    }
    relay_api_unhook_config_options (client);
}

/*
 * Read HTTP request from a client.
 */

void
relay_api_recv_http (struct t_relay_client *client)
{
    relay_api_protocol_recv_http (client);
}

/*
 * Read JSON string from a client.
 */

void
relay_api_recv_json (struct t_relay_client *client, const char *json)
{
    relay_api_protocol_recv_json (client, json);
}

/*
 * Close connection with a client.
 */

void
relay_api_close_connection (struct t_relay_client *client)
{
    /*
     * IMPORTANT: if changes are made in this function or sub-functions called,
     * please also update the function relay_api_add_to_infolist:
     * when the flag force_disconnected_state is set to 1 we simulate
     * a disconnected state for client in infolist (used on /upgrade -save).
     */

    relay_api_unhook_signals (client);
}

/*
 * Initialize relay data specific to API protocol.
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
    RELAY_API_DATA(client, hook_signal_day_changed) = NULL;
    RELAY_API_DATA(client, hook_config_options) = NULL;
    RELAY_API_DATA(client, buffers_closing) = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_POINTER,
        WEECHAT_HASHTABLE_LONGLONG,
        NULL,
        NULL);
    RELAY_API_DATA(client, sync_enabled) = 0;
    RELAY_API_DATA(client, sync_nicks) = 0;
    RELAY_API_DATA(client, sync_input) = 0;
    RELAY_API_DATA(client, sync_notify_only) = 0;
    RELAY_API_DATA(client, sync_colors) = RELAY_API_COLORS_ANSI;
    RELAY_API_DATA(client, sync_rgb_colors) = 0;
    RELAY_API_DATA(client, sync_options) = NULL;
    RELAY_API_DATA(client, num_sync_options) = 0;
}

/*
 * Initialize relay data specific to API protocol with an infolist.
 *
 * This is called after /upgrade.
 */

void
relay_api_alloc_with_infolist (struct t_relay_client *client,
                               struct t_infolist *infolist)
{
    char **options;
    int i, num_options;

    client->protocol_data = malloc (sizeof (struct t_relay_api_data));
    if (!client->protocol_data)
        return;

    RELAY_API_DATA(client, hook_signal_buffer) = NULL;
    RELAY_API_DATA(client, hook_hsignal_nicklist) = NULL;
    RELAY_API_DATA(client, hook_signal_input) = NULL;
    RELAY_API_DATA(client, hook_signal_upgrade) = NULL;
    RELAY_API_DATA(client, hook_signal_day_changed) = NULL;
    RELAY_API_DATA(client, hook_config_options) = NULL;
    RELAY_API_DATA(client, buffers_closing) = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_POINTER,
        WEECHAT_HASHTABLE_LONGLONG,
        NULL,
        NULL);
    RELAY_API_DATA(client, sync_enabled) = weechat_infolist_integer (
        infolist, "sync_enabled");
    RELAY_API_DATA(client, sync_nicks) = weechat_infolist_integer (
        infolist, "sync_nicks");
    RELAY_API_DATA(client, sync_input) = weechat_infolist_integer (
        infolist, "sync_input");
    RELAY_API_DATA(client, sync_notify_only) = weechat_infolist_integer (
        infolist, "sync_notify_only");
    RELAY_API_DATA(client, sync_colors) = weechat_infolist_integer (
        infolist, "sync_colors");
    RELAY_API_DATA(client, sync_rgb_colors) = weechat_infolist_integer (
        infolist, "sync_rgb_colors");
    RELAY_API_DATA(client, sync_options) = NULL;
    RELAY_API_DATA(client, num_sync_options) = 0;
    options = weechat_string_split (
        weechat_infolist_string (infolist, "sync_options"),
        "\n",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &num_options);
    if (options)
    {
        for (i = 0; i < num_options; i++)
        {
            relay_api_add_sync_option (client, options[i]);
        }
        weechat_string_free_split (options);
    }

    if (!RELAY_STATUS_HAS_ENDED(client->status)
        && RELAY_API_DATA(client, sync_enabled))
    {
        relay_api_hook_signals (client);
    }
}

/*
 * Return the client initial status: it is always "authenticating" for API
 * protocol because we always expect the client to authenticate.
 */

enum t_relay_status
relay_api_get_initial_status (struct t_relay_client *client)
{
    /* Make C compiler happy. */
    (void) client;

    return RELAY_STATUS_AUTHENTICATING;
}

/*
 * Free relay data specific to API protocol.
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
        weechat_unhook (RELAY_API_DATA(client, hook_signal_day_changed));
        relay_api_unhook_config_options (client);
        weechat_hashtable_free (RELAY_API_DATA(client, buffers_closing));
        relay_api_free_sync_options (client);

        free (client->protocol_data);

        client->protocol_data = NULL;
    }
}

/*
 * Add client API data in an infolist.
 *
 * If force_disconnected_state == 1, the infolist contains the client
 * in a disconnected state (but the client is unchanged, still connected if it
 * was).
 *
 * Return:
 *   1: OK
 *   0: error
 */

int
relay_api_add_to_infolist (struct t_infolist_item *item,
                           struct t_relay_client *client,
                           int force_disconnected_state)
{
    char *options;
    int rc;

    if (!item || !client)
        return 0;

    /* Parameter not used today, it may be in future. */
    (void) force_disconnected_state;

    if (!weechat_infolist_new_var_pointer (item, "hook_signal_buffer", RELAY_API_DATA(client, hook_signal_buffer)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_hsignal_nicklist", RELAY_API_DATA(client, hook_hsignal_nicklist)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_input", RELAY_API_DATA(client, hook_signal_input)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_upgrade", RELAY_API_DATA(client, hook_signal_upgrade)))
        return 0;
    if (!weechat_infolist_new_var_pointer (item, "hook_signal_day_changed", RELAY_API_DATA(client, hook_signal_day_changed)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_enabled", RELAY_API_DATA(client, sync_enabled)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_nicks", RELAY_API_DATA(client, sync_nicks)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_input", RELAY_API_DATA(client, sync_input)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_notify_only", RELAY_API_DATA(client, sync_notify_only)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_colors", RELAY_API_DATA(client, sync_colors)))
        return 0;
    if (!weechat_infolist_new_var_integer (item, "sync_rgb_colors", RELAY_API_DATA(client, sync_rgb_colors)))
        return 0;
    options = (RELAY_API_DATA(client, sync_options)) ?
        weechat_string_rebuild_split_string (
            (const char **)RELAY_API_DATA(client, sync_options), "\n", 0, -1) :
        NULL;
    rc = (weechat_infolist_new_var_string (item, "sync_options", options)) ? 1 : 0;
    free (options);
    if (!rc)
        return 0;

    return 1;
}

/*
 * Print client API data in WeeChat log file (usually for crash dump).
 */

void
relay_api_print_log (struct t_relay_client *client)
{
    int i;

    if (client->protocol_data)
    {
        weechat_log_printf ("    hook_signal_buffer. . . : %p", RELAY_API_DATA(client, hook_signal_buffer));
        weechat_log_printf ("    hook_hsignal_nicklist . : %p", RELAY_API_DATA(client, hook_hsignal_nicklist));
        weechat_log_printf ("    hook_signal_input . . . : %p", RELAY_API_DATA(client, hook_signal_input));
        weechat_log_printf ("    hook_signal_upgrade . . : %p", RELAY_API_DATA(client, hook_signal_upgrade));
        weechat_log_printf ("    hook_signal_day_changed : %p", RELAY_API_DATA(client, hook_signal_day_changed));
        weechat_log_printf ("    buffers_closing. . . . .: %p (hashtable: '%s')",
                            RELAY_API_DATA(client, buffers_closing),
                            weechat_hashtable_get_string (
                                RELAY_API_DATA(client, buffers_closing),
                                "keys_values"));
        weechat_log_printf ("    sync_enabled. . . . . . : %d", RELAY_API_DATA(client, sync_enabled));
        weechat_log_printf ("    sync_nicks. . . . . . . : %d", RELAY_API_DATA(client, sync_nicks));
        weechat_log_printf ("    sync_input. . . . . . . : %d", RELAY_API_DATA(client, sync_input));
        weechat_log_printf ("    sync_notify_only. . . . : %d", RELAY_API_DATA(client, sync_notify_only));
        weechat_log_printf ("    sync_colors . . . . . . : %d", RELAY_API_DATA(client, sync_colors));
        weechat_log_printf ("    sync_rgb_colors . . . . : %d", RELAY_API_DATA(client, sync_rgb_colors));
        weechat_log_printf ("    num_sync_options. . . . : %d", RELAY_API_DATA(client, num_sync_options));
        for (i = 0; i < RELAY_API_DATA(client, num_sync_options); i++)
        {
            weechat_log_printf ("      sync_options[%d]. . . : '%s' (hook: %p)",
                                i,
                                RELAY_API_DATA(client, sync_options)[i],
                                (RELAY_API_DATA(client, hook_config_options)) ?
                                RELAY_API_DATA(client, hook_config_options)[i] : NULL);
        }
    }
}
