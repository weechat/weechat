/*
 * relay.c - network communication between WeeChat and remote client
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-command.h"
#include "relay-completion.h"
#include "relay-config.h"
#include "relay-info.h"
#include "relay-network.h"
#include "relay-raw.h"
#include "relay-remote.h"
#include "relay-server.h"
#include "relay-upgrade.h"


WEECHAT_PLUGIN_NAME(RELAY_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Relay WeeChat data to remote application "
                              "(irc/weechat protocols)"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(RELAY_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_relay_plugin = NULL;

char *relay_protocol_string[] =        /* strings for protocols             */
{ "weechat", "irc", "api" };

char *relay_status_string[] =          /* status strings for display        */
{ N_("connecting"), N_("authenticating"),
  N_("connected"), N_("authentication failed"), N_("disconnected")
};
char *relay_status_name[] =            /* name of status (for signal/info)  */
{ "connecting", "waiting_auth",
  "connected", "auth_failed", "disconnected"
};
char *relay_msg_type_string[] =        /* prefix in raw buffer for msg      */
{ "", "[PING]\n", "[PONG]\n", "[CLOSE]\n" };

struct t_hdata *relay_hdata_buffer = NULL;
struct t_hdata *relay_hdata_key = NULL;
struct t_hdata *relay_hdata_lines = NULL;
struct t_hdata *relay_hdata_line = NULL;
struct t_hdata *relay_hdata_line_data = NULL;
struct t_hdata *relay_hdata_nick_group = NULL;
struct t_hdata *relay_hdata_nick = NULL;
struct t_hdata *relay_hdata_completion = NULL;
struct t_hdata *relay_hdata_completion_word = NULL;
struct t_hdata *relay_hdata_hotlist = NULL;

int relay_signal_upgrade_received = 0; /* signal "upgrade" received ?       */

struct t_hook *relay_hook_timer = NULL;


/*
 * Searches for a protocol.
 *
 * Returns index of protocol in enum t_relay_protocol, -1 if protocol is not
 * found.
 */

int
relay_protocol_search (const char *name)
{
    int i;

    if (!name)
        return -1;

    for (i = 0; i < RELAY_NUM_PROTOCOLS; i++)
    {
        if (strcmp (relay_protocol_string[i], name) == 0)
            return i;
    }

    /* protocol not found */
    return -1;
}

/*
 * Searches for a status.
 *
 * Returns index of status in enum t_relay_status, -1 if status is not found.
 */

int
relay_status_search (const char *name)
{
    int i;

    if (!name)
        return -1;

    for (i = 0; i < RELAY_NUM_STATUS; i++)
    {
        if (strcmp (relay_status_name[i], name) == 0)
            return i;
    }

    /* status not found */
    return -1;
}

/*
 * Callback for signal "upgrade".
 */

int
relay_signal_upgrade_cb (const void *pointer, void *data,
                         const char *signal, const char *type_data,
                         void *signal_data)
{
    struct t_relay_server *ptr_server;
    struct t_relay_client *ptr_client;
    int quit, tls_disconnected;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    /* only save session and continue? */
    if (signal_data && (strcmp (signal_data, "save") == 0))
    {
        /* save session with a disconnected state in clients */
        relay_upgrade_save (1);
        return WEECHAT_RC_OK;
    }

    relay_signal_upgrade_received = 1;

    /* close socket for relay servers */
    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        relay_server_close_socket (ptr_server);
    }

    quit = (signal_data && (strcmp (signal_data, "quit") == 0));
    tls_disconnected = 0;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        /*
         * FIXME: it's not possible to upgrade with TLS clients connected (GnuTLS
         * lib can't reload data after upgrade), so we close connection for
         * all TLS clients currently connected
         */
        if ((ptr_client->sock >= 0) && (ptr_client->tls || quit))
        {
            if (!quit)
            {
                tls_disconnected++;
                weechat_printf (NULL,
                                _("%s%s: disconnecting from client %s%s%s because "
                                  "upgrade can't work for clients connected via TLS"),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                RELAY_COLOR_CHAT_CLIENT,
                                ptr_client->desc,
                                RELAY_COLOR_CHAT);
            }
            relay_client_set_status (ptr_client, RELAY_STATUS_DISCONNECTED);
        }
    }
    if (tls_disconnected > 0)
    {
        weechat_printf (NULL,
                        /* TRANSLATORS: "%s" after "%d" is "client" or "clients" */
                        _("%s%s: disconnected from %d %s (TLS connection "
                          "not supported with upgrade)"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                        tls_disconnected,
                        NG_("client", "clients", tls_disconnected));
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for signal "debug_dump".
 */

int
relay_debug_dump_cb (const void *pointer, void *data,
                     const char *signal, const char *type_data,
                     void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    if (!signal_data || (strcmp ((char *)signal_data, RELAY_PLUGIN_NAME) == 0))
    {
        weechat_log_printf ("");
        weechat_log_printf ("***** \"%s\" plugin dump *****",
                            weechat_plugin->name);

        relay_server_print_log ();
        relay_client_print_log ();
        relay_remote_print_log ();

        weechat_log_printf ("");
        weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                            weechat_plugin->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Initializes relay plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    int auto_connect;
    char *info_auto_connect;

    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    relay_hdata_buffer = weechat_hdata_get ("buffer");
    relay_hdata_key = weechat_hdata_get ("key");
    relay_hdata_lines = weechat_hdata_get ("lines");
    relay_hdata_line = weechat_hdata_get ("line");
    relay_hdata_line_data = weechat_hdata_get ("line_data");
    relay_hdata_nick_group = weechat_hdata_get ("nick_group");
    relay_hdata_nick = weechat_hdata_get ("nick");
    relay_hdata_completion = weechat_hdata_get ("completion");
    relay_hdata_completion_word = weechat_hdata_get ("completion_word");
    relay_hdata_hotlist = weechat_hdata_get ("hotlist");

    relay_signal_upgrade_received = 0;

    if (!relay_config_init ())
        return WEECHAT_RC_ERROR;

    relay_config_read ();

    relay_network_init ();

    relay_command_init ();

    /* hook completions */
    relay_completion_init ();

    weechat_hook_signal ("upgrade", &relay_signal_upgrade_cb, NULL, NULL);
    weechat_hook_signal ("debug_dump", &relay_debug_dump_cb, NULL, NULL);

    relay_info_init ();

    if (weechat_relay_plugin->upgrading)
    {
        relay_upgrade_load ();
    }
    else
    {
        /* check if auto-connect is enabled */
        info_auto_connect = weechat_info_get ("auto_connect", NULL);
        auto_connect = (info_auto_connect && (strcmp (info_auto_connect, "1") == 0)) ?
            1 : 0;
        free (info_auto_connect);
        if (auto_connect)
            relay_remote_auto_connect ();
    }

    relay_hook_timer = weechat_hook_timer (1 * 1000, 0, 0,
                                           &relay_client_timer_cb, NULL, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Ends relay plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    if (relay_hook_timer)
    {
        weechat_unhook (relay_hook_timer);
        relay_hook_timer = NULL;
    }

    relay_config_write ();

    if (relay_signal_upgrade_received)
        relay_upgrade_save (0);
    else
        relay_client_disconnect_all ();

    relay_raw_message_free_all ();

    relay_server_free_all ();

    if (relay_buffer)
    {
        weechat_buffer_close (relay_buffer);
        relay_buffer = NULL;
    }
    relay_buffer_selected_line = 0;

    relay_client_free_all ();

    relay_network_end ();

    relay_config_free ();

    return WEECHAT_RC_OK;
}
