/*
 * relay-command.c - relay commands
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
#include <limits.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-network.h"
#include "relay-raw.h"
#include "relay-remote.h"
#include "relay-server.h"


/*
 * Displays list of clients.
 */

void
relay_command_client_list (int full)
{
    struct t_relay_client *ptr_client;
    char date_start[128], date_activity[128];
    struct tm *date_tmp;
    int num_found;

    num_found = 0;
    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (!full && RELAY_STATUS_HAS_ENDED(ptr_client->status))
            continue;

        if (num_found == 0)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL,
                            (full) ?
                            _("Clients for relay:") :
                            _("Connected clients for relay:"));
        }
        num_found++;

        date_start[0] = '\0';
        date_tmp = localtime (&(ptr_client->start_time));
        if (date_tmp)
        {
            if (strftime (date_start, sizeof (date_start),
                          "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                date_start[0] = '\0';
        }

        date_activity[0] = '\0';
        date_tmp = localtime (&(ptr_client->last_activity));
        if (date_tmp)
        {
            if (strftime (date_activity, sizeof (date_activity),
                          "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                date_activity[0] = '\0';
        }

        if (full)
        {
            weechat_printf (NULL,
                            _("  %s%s%s (%s%s%s), started on: %s, last activity: %s, "
                              "bytes: %llu recv, %llu sent"),
                            RELAY_COLOR_CHAT_CLIENT,
                            ptr_client->desc,
                            RELAY_COLOR_CHAT,
                            weechat_color (weechat_config_string (relay_config_color_status[ptr_client->status])),
                            relay_status_string[ptr_client->status],
                            RELAY_COLOR_CHAT,
                            date_start,
                            date_activity,
                            ptr_client->bytes_recv,
                            ptr_client->bytes_sent);
        }
        else
        {
            weechat_printf (NULL,
                            _("  %s%s%s (%s%s%s), started on: %s"),
                            RELAY_COLOR_CHAT_CLIENT,
                            ptr_client->desc,
                            RELAY_COLOR_CHAT,
                            weechat_color (weechat_config_string (relay_config_color_status[ptr_client->status])),
                            relay_status_string[ptr_client->status],
                            RELAY_COLOR_CHAT,
                            date_start);
        }
    }

    if (num_found == 0)
    {
        weechat_printf (NULL,
                        (full) ?
                        _("No client for relay") :
                        _("No connected client for relay"));
    }
}

/*
 * Displays list of servers (list of ports on which we are listening).
 */

void
relay_command_server_list ()
{
    struct t_relay_server *ptr_server;
    char date_start[128];
    struct tm *date_tmp;

    if (relay_servers)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Listening on:"));
        for (ptr_server = relay_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->sock < 0)
            {
                weechat_printf (
                    NULL,
                    _("  %s %s%s%s, relay: %s%s%s, %s (not started)"),
                    (ptr_server->unix_socket) ? _("path") : _("port"),
                    RELAY_COLOR_CHAT_BUFFER,
                    ptr_server->path,
                    RELAY_COLOR_CHAT,
                    RELAY_COLOR_CHAT_BUFFER,
                    ptr_server->protocol_string,
                    RELAY_COLOR_CHAT,
                    ((ptr_server->ipv4 && ptr_server->ipv6) ? "IPv4+6" : ((ptr_server->ipv6) ? "IPv6" : ((ptr_server->ipv4) ? "IPv4" : "UNIX"))));
            }
            else
            {
                date_start[0] = '\0';
                date_tmp = localtime (&(ptr_server->start_time));
                if (date_tmp)
                {
                    if (strftime (date_start, sizeof (date_start),
                                  "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                        date_start[0] = '\0';
                }
                weechat_printf (
                    NULL,
                    _("  %s %s%s%s, relay: %s%s%s, %s, started on: %s"),
                    (ptr_server->unix_socket) ? _("path") : _("port"),
                    RELAY_COLOR_CHAT_BUFFER,
                    ptr_server->path,
                    RELAY_COLOR_CHAT,
                    RELAY_COLOR_CHAT_BUFFER,
                    ptr_server->protocol_string,
                    RELAY_COLOR_CHAT,
                    ((ptr_server->ipv4 && ptr_server->ipv6) ? "IPv4+6" : ((ptr_server->ipv6) ? "IPv6" : ((ptr_server->ipv4) ? "IPv4" : "UNIX"))),
                    date_start);
            }
        }
    }
    else
        weechat_printf (NULL, _("No server for relay"));
}

/*
 * Callback for command "/relay".
 */

int
relay_command_relay (const void *pointer, void *data,
                     struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    struct t_relay_server *ptr_server;
    struct t_config_option *ptr_option;
    char *path;
    int unix_socket, rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if (argc > 1)
    {
        if (weechat_strcmp (argv[1], "list") == 0)
        {
            relay_command_client_list (0);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "listfull") == 0)
        {
            relay_command_client_list (1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "listrelay") == 0)
        {
            relay_command_server_list ();
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "add") == 0)
        {
            WEECHAT_COMMAND_MIN_ARGS(4, "add");
            relay_server_get_protocol_args (argv[2], NULL, NULL, NULL,
                                            &unix_socket, NULL, NULL);
            rc = relay_config_create_option_port_path (
                NULL, NULL,
                relay_config_file,
                (unix_socket) ? relay_config_section_path : relay_config_section_port,
                argv[2],
                argv_eol[3]);
            if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
            {
                weechat_printf (NULL,
                                _("%s: relay \"%s\" (%s: %s) added"),
                                RELAY_PLUGIN_NAME,
                                argv[2],
                                (unix_socket) ? _("path") : _("port"),
                                argv_eol[3]);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "del") == 0)
        {
            WEECHAT_COMMAND_MIN_ARGS(3, "del");
            ptr_server = relay_server_search (argv_eol[2]);
            if (ptr_server)
            {
                unix_socket = ptr_server->unix_socket;
                path = strdup (ptr_server->path);
                relay_server_free (ptr_server);
                ptr_option = weechat_config_search_option (
                    relay_config_file,
                    (unix_socket) ? relay_config_section_path : relay_config_section_port,
                    argv_eol[2]);
                weechat_config_option_free (ptr_option);
                weechat_printf (NULL,
                                _("%s: relay \"%s\" (%s: %s) removed"),
                                RELAY_PLUGIN_NAME,
                                argv[2],
                                (unix_socket) ? _("path") : _("port"),
                                path);
                free (path);
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: relay \"%s\" not found"),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                argv_eol[2]);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "stop") == 0)
        {
            WEECHAT_COMMAND_MIN_ARGS(3, "stop");
            ptr_server = relay_server_search (argv_eol[2]);
            if (ptr_server)
            {
                relay_server_close_socket (ptr_server);
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: relay \"%s\" not found"),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                argv_eol[2]);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "start") == 0)
        {
            WEECHAT_COMMAND_MIN_ARGS(3, "start");
            ptr_server = relay_server_search (argv_eol[2]);
            if (ptr_server)
            {
                if (ptr_server->sock < 0)
                    relay_server_create_socket (ptr_server);
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: relay \"%s\" not found"),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                argv_eol[2]);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "restart") == 0)
        {
            WEECHAT_COMMAND_MIN_ARGS(3, "restart");
            ptr_server = relay_server_search (argv_eol[2]);
            if (ptr_server)
            {
                relay_server_close_socket (ptr_server);
                relay_server_create_socket (ptr_server);
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: relay \"%s\" not found"),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                argv_eol[2]);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "raw") == 0)
        {
            relay_raw_open (1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "tlscertkey") == 0)
        {
            relay_network_set_tls_cert_key (1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "up") == 0)
        {
            if (relay_buffer && (relay_buffer_selected_line > 0))
            {
                relay_buffer_selected_line--;
                relay_buffer_refresh (NULL);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "down") == 0)
        {
            if (relay_buffer
                && relay_buffer_selected_line < relay_client_count - 1)
            {
                relay_buffer_selected_line++;
                relay_buffer_refresh (NULL);
            }
            return WEECHAT_RC_OK;
        }

        WEECHAT_COMMAND_ERROR;
    }

    if (!relay_buffer)
        relay_buffer_open ();

    if (relay_buffer)
    {
        weechat_buffer_set (relay_buffer, "display", "1");
        relay_buffer_refresh (NULL);
    }

    return WEECHAT_RC_OK;
}

/*
 * Displays a relay remote.
 */

void
relay_command_display_remote (struct t_relay_remote *remote, int with_detail)
{
    if (with_detail)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Remote: %s"), remote->name);
        weechat_printf (NULL, "  url. . . . . . . . . : '%s'",
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]));
        weechat_printf (NULL, "  autoconnect. . . . . : %s",
                        (weechat_config_string (remote->options[RELAY_REMOTE_OPTION_AUTOCONNECT])) ?
                        "on" : "off");
        weechat_printf (NULL, "  proxy. . . . . . . . : '%s'",
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_PROXY]));
        weechat_printf (NULL, "  tls_verify . . . . . : %s",
                        (weechat_config_string (remote->options[RELAY_REMOTE_OPTION_TLS_VERIFY])) ?
                        "on" : "off");
        weechat_printf (NULL, "  password . . . . . . : '%s'",
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_PASSWORD]));
        weechat_printf (NULL, "  totp_secret. . . . . : '%s'",
                        weechat_config_string (remote->options[RELAY_REMOTE_OPTION_TOTP_SECRET]));
    }
    else
    {
        weechat_printf (
            NULL,
            "  %s: %s",
            remote->name,
            weechat_config_string (remote->options[RELAY_REMOTE_OPTION_URL]));
    }
}

/*
 * Callback for command "/remote".
 */

int
relay_command_remote (const void *pointer, void *data,
                      struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    struct t_relay_remote *ptr_remote, *ptr_remote2;
    int i, detailed_list, one_remote_found;
    const char *ptr_autoconnect, *ptr_proxy, *ptr_tls_verify, *ptr_password;
    const char *ptr_totp_secret;
    char *remote_name;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if ((argc == 1)
        || (weechat_strcmp (argv[1], "list") == 0)
        || (weechat_strcmp (argv[1], "listfull") == 0))
    {
        /* list remotes */
        remote_name = NULL;
        detailed_list = 0;
        for (i = 1; i < argc; i++)
        {
            if (weechat_strcmp (argv[i], "list") == 0)
                continue;
            if (weechat_strcmp (argv[i], "listfull") == 0)
            {
                detailed_list = 1;
                continue;
            }
            if (!remote_name)
                remote_name = argv[i];
        }
        if (remote_name)
        {
            one_remote_found = 0;
            for (ptr_remote = relay_remotes; ptr_remote;
                 ptr_remote = ptr_remote->next_remote)
            {
                if (strstr (ptr_remote->name, remote_name))
                {
                    if (!one_remote_found)
                    {
                        weechat_printf (NULL, "");
                        weechat_printf (NULL,
                                        _("Relay remotes with \"%s\":"),
                                        remote_name);
                    }
                    one_remote_found = 1;
                    relay_command_display_remote (ptr_remote, detailed_list);
                }
            }
            if (!one_remote_found)
            {
                weechat_printf (NULL,
                                _("No relay remote found with \"%s\""),
                                remote_name);
            }
        }
        else
        {
            if (relay_remotes)
            {
                weechat_printf (NULL, "");
                weechat_printf (NULL, _("All relay remotes:"));
                for (ptr_remote = relay_remotes; ptr_remote;
                     ptr_remote = ptr_remote->next_remote)
                {
                    relay_command_display_remote (ptr_remote, detailed_list);
                }
            }
            else
                weechat_printf (NULL, _("No relay remote"));
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "add") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "add");
        ptr_remote = relay_remote_search (argv[2]);
        if (ptr_remote)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" already exists, can't add it!"),
                weechat_prefix ("error"), RELAY_PLUGIN_NAME, ptr_remote->name);
            return WEECHAT_RC_OK;
        }
        if (!relay_remote_name_valid (argv[2]))
        {
            weechat_printf (NULL,
                            _("%s%s: invalid remote relay name: \"%s\""),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME,
                            argv[2]);
            return WEECHAT_RC_OK;
        }
        if (!relay_remote_url_valid (argv[3]))
        {
            weechat_printf (NULL,
                            _("%s%s: invalid remote relay URL: \"%s\""),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME,
                            argv[3]);
            return WEECHAT_RC_OK;
        }
        ptr_autoconnect = NULL;
        ptr_proxy = NULL;
        ptr_tls_verify = NULL;
        ptr_password = NULL;
        ptr_totp_secret = NULL;
        for (i = 4; i < argc; i++)
        {
            if (strncmp (argv[i], "-autoconnect=", 13) == 0)
            {
                ptr_autoconnect = argv[i] + 13;
            }
            else if (strncmp (argv[i], "-proxy=", 7) == 0)
            {
                ptr_proxy = argv[i] + 7;
            }
            else if (strncmp (argv[i], "-tls_verify=", 12) == 0)
            {
                ptr_tls_verify = argv[i] + 12;
            }
            else if (strncmp (argv[i], "-password=", 10) == 0)
            {
                ptr_password = argv[i] + 10;
            }
            else if (strncmp (argv[i], "-totp_secret=", 13) == 0)
            {
                ptr_totp_secret = argv[i] + 13;
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: invalid remote relay option: \"%s\""),
                                weechat_prefix ("error"),
                                RELAY_PLUGIN_NAME,
                                argv[i]);
                return WEECHAT_RC_OK;
            }
        }
        ptr_remote = relay_remote_new (argv[2], argv[3], ptr_autoconnect,
                                       ptr_proxy, ptr_tls_verify, ptr_password,
                                       ptr_totp_secret);
        if (ptr_remote)
        {
            weechat_printf (NULL, _("Remote relay \"%s\" created"), argv[2]);
        }
        else
        {
            weechat_printf (
                NULL,
                _("%s%s: failed to create remote relay \"%s\""),
                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                argv[2]);
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "connect") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "connect");
        ptr_remote = relay_remote_search (argv[2]);
        if (!ptr_remote)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2],
                "remote connect");
            return WEECHAT_RC_OK;
        }
        relay_remote_connect (ptr_remote);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "send") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "send");
        ptr_remote = relay_remote_search (argv[2]);
        if (!ptr_remote)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2],
                "remote send");
            return WEECHAT_RC_OK;
        }
        if (ptr_remote->status != RELAY_STATUS_CONNECTED)
        {
            weechat_printf (
                NULL,
                _("%s%s: no connection to remote relay \"%s\""),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2]);
            return WEECHAT_RC_OK;
        }
        relay_remote_send (ptr_remote, argv_eol[3]);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "disconnect") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "disconnect");
        ptr_remote = relay_remote_search (argv[2]);
        if (!ptr_remote)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2],
                "remote disconnect");
            return WEECHAT_RC_OK;
        }
        relay_remote_disconnect (ptr_remote);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "rename") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "rename");
        /* look for remote by name */
        ptr_remote = relay_remote_search (argv[2]);
        if (!ptr_remote)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2],
                "remote rename");
            return WEECHAT_RC_OK;
        }
        /* check if target name already exists */
        ptr_remote2 = relay_remote_search (argv[3]);
        if (ptr_remote2)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" already exists for \"%s\" command"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                ptr_remote2->name,
                "server rename");
            return WEECHAT_RC_OK;
        }
        /* rename remote */
        if (relay_remote_rename (ptr_remote, argv[3]))
        {
            weechat_printf (
                NULL,
                _("%s: remote relay \"%s\" has been renamed to \"%s\""),
                RELAY_PLUGIN_NAME,
                argv[2],
                argv[3]);
            return WEECHAT_RC_OK;
        }
        WEECHAT_COMMAND_ERROR;
    }

    if (weechat_strcmp (argv[1], "del") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "del");
        /* look for remote by name */
        ptr_remote = relay_remote_search (argv[2]);
        if (!ptr_remote)
        {
            weechat_printf (
                NULL,
                _("%s%s: remote relay \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2],
                "remote del");
            return WEECHAT_RC_OK;
        }
        if (!RELAY_STATUS_HAS_ENDED(ptr_remote->status))
        {
            weechat_printf (
                NULL,
                _("%s%s: you can not delete remote relay \"%s\" because you are "
                  "connected to. Try \"/remote disconnect %s\" before."),
                weechat_prefix ("error"),
                RELAY_PLUGIN_NAME,
                argv[2],
                argv[2]);
            return WEECHAT_RC_OK;
        }
        remote_name = strdup (ptr_remote->name);
        relay_remote_free (ptr_remote);
        weechat_printf (
            NULL,
            _("%s: remote relay \"%s\" has been deleted"),
            RELAY_PLUGIN_NAME,
            (remote_name) ? remote_name : "???");
        free (remote_name);
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks command.
 */

void
relay_command_init ()
{
    weechat_hook_command (
        "relay",
        N_("relay control"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("list|listfull|listrelay"
           " || add <name> <port>|<path>"
           " || del|start|restart|stop <name>"
           " || raw"
           " || tlscertkey"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[list]: list relay clients (only active relays)"),
            N_("raw[listfull]: list relay clients (verbose, all relays)"),
            N_("raw[listrelay]: list relays (name and port)"),
            N_("raw[add]: add a relay (listen on a port/path)"),
            N_("raw[del]: remove a relay (clients remain connected)"),
            N_("raw[start]: listen on port"),
            N_("raw[restart]: close the server socket and listen again on port "
               "(clients remain connected)"),
            N_("raw[stop]: close the server socket (clients remain connected)"),
            N_("name: relay name (see format below)"),
            N_("port: port used for relay"),
            N_("path: path used for relay (for UNIX domain socket only); "
               "path is evaluated (see function string_eval_path_home in "
               "plugin API reference)"),
            N_("raw[raw]: open buffer with raw Relay data"),
            N_("raw[tlscertkey]: set TLS certificate/key using path in option "
               "relay.network.tls_cert_key"),
            "",
            N_("Relay name is: [ipv4.][ipv6.][tls.]<protocol.name> or "
               "unix.[tls.]<protocol.name>:"),
            N_("  - ipv4: force use of IPv4"),
            N_("  - ipv6: force use of IPv6"),
            N_("  - tls: enable TLS"),
            N_("  - unix: use UNIX domain socket"),
            N_("  - protocol.name: protocol and name to relay:"),
            N_("    - protocol \"irc\": name is the server to share "
               "(optional, if not given, the server name must be sent by client in "
               "command \"PASS\", with format: \"PASS server:password\")"),
            N_("    - protocol \"api\" (name is not used)"),
            N_("    - protocol \"weechat\" (name is not used)"),
            "",
            N_("The \"irc\" protocol allows any IRC client (including WeeChat "
               "itself) to connect on the port."),
            N_("The \"api\" protocol allows a remote interface (including "
               "WeeChat itself) to connect on the port."),
            N_("The \"weechat\" protocol allows a remote interface "
               "(but not WeeChat itself) to connect on the port."),
            "",
            N_("The list of remote interfaces is here: "
               "https://weechat.org/about/interfaces/"),
            "",
            N_("Without argument, this command opens buffer with list of relay "
               "clients."),
            "",
            N_("Examples:"),
            AI("  /relay add irc.libera 8000"),
            AI("  /relay add tls.irc.libera 8001"),
            AI("  /relay add tls.irc 8002"),
            AI("  /relay add tls.api 9000"),
            AI("  /relay add weechat 10000"),
            AI("  /relay add tls.weechat 10001"),
            AI("  /relay add ipv4.tls.weechat 10001"),
            AI("  /relay add ipv6.tls.weechat 10001"),
            AI("  /relay add ipv4.ipv6.tls.weechat 10001"),
            AI("  /relay add unix.weechat ${weechat_runtime_dir}/relay_socket")),
        "list %(relay_relays)"
        " || listfull %(relay_relays)"
        " || listrelay"
        " || add %(relay_protocol_name) %(relay_free_port)"
        " || del %(relay_relays)"
        " || start %(relay_relays)"
        " || restart %(relay_relays)"
        " || stop %(relay_relays)"
        " || raw"
        " || tlscertkey",
        &relay_command_relay, NULL, NULL);
    weechat_hook_command (
        "remote",
        N_("control of remote relay servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("list|listfull [<name>]"
           " || add <name> <url> [-<option>[=<value>]]"
           " || connect <name>"
           " || send <name> <json>"
           " || disconnect <name>"
           " || rename <name> <new_name>"
           " || del <name>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[list]: list remote relay servers "
               "(without argument, this list is displayed)"),
            N_("raw[listfull]: list remote relay servers (verbose)"),
            N_("raw[add]: add a remote relay server"),
            N_("name: name of remote relay server, for internal and display use; "
               "this name is used to connect to the remote relay and to set "
               "remote relay options: relay.remote.name.xxx"),
            N_("url: URL of the remote relay, format is https://example.com:9000 "
               "or http://example.com:9000 (plain-text connection, not recommended)"),
            N_("option: set option for remote relay"),
            N_("raw[connect]: connect to a remote relay server"),
            N_("raw[send]: send JSON data to a remote relay server"),
            N_("raw[disconnect]: disconnect from a remote relay server"),
            N_("raw[rename]: rename a remote relay server"),
            N_("raw[del]: delete a remote relay server"),
            "",
            N_("Examples:"),
            AI("  /remote add example https://localhost:9000 "
               "-password=my_secret_password -totp_secret=secrettotp"),
            AI("  /remote connect example"),
            AI("  /remote del example")),
        "list %(relay_remotes)"
        " || listfull %(relay_remotes)"
        " || add %(relay_remotes) https://localhost:9000 "
        "-autoconnect=on|-password=${xxx}|-proxy=xxx|-tls_verify=off|"
        "-totp_secret=${xxx}|%*"
        " || connect %(relay_remotes)"
        " || send %(relay_remotes) {\"request\":\"\"}"
        " || disconnect %(relay_remotes)"
        " || rename %(relay_remotes) %(relay_remotes)"
        " || del %(relay_remotes)",
        &relay_command_remote, NULL, NULL);
}
