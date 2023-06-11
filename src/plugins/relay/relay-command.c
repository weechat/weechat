/*
 * relay-command.c - relay command
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
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-buffer.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-network.h"
#include "relay-raw.h"
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
        if (!full && RELAY_CLIENT_HAS_ENDED(ptr_client))
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
                            relay_client_status_string[ptr_client->status],
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
                            relay_client_status_string[ptr_client->status],
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
    int i;
    char date_start[128];
    struct tm *date_tmp;

    if (relay_servers)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Listening on:"));
        i = 1;
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
            i++;
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
                if (ptr_option)
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
 * Hooks command.
 */

void
relay_command_init ()
{
    weechat_hook_command (
        "relay",
        N_("relay control"),
        N_("list|listfull|listrelay"
           " || add <name> <port>|<path>"
           " || del|start|restart|stop <name>"
           " || raw"
           " || tlscertkey"),
        N_("         list: list relay clients (only active relays)\n"
           "     listfull: list relay clients (verbose, all relays)\n"
           "    listrelay: list relays (name and port)\n"
           "          add: add a relay (listen on a port/path)\n"
           "          del: remove a relay (clients remain connected)\n"
           "        start: listen on port\n"
           "      restart: close the server socket and listen again on port "
           "(clients remain connected)\n"
           "         stop: close the server socket (clients remain connected)\n"
           "         name: relay name (see format below)\n"
           "         port: port used for relay\n"
           "         path: path used for relay (for UNIX domain socket only); "
           "path is evaluated (see function string_eval_path_home in "
           "plugin API reference)\n"
           "          raw: open buffer with raw Relay data\n"
           "   tlscertkey: set TLS certificate/key using path in option "
           "relay.network.tls_cert_key\n"
           "\n"
           "Relay name is: [ipv4.][ipv6.][tls.]<protocol.name> or "
           "unix.[tls.]<protocol.name>\n"
           "         ipv4: force use of IPv4\n"
           "         ipv6: force use of IPv6\n"
           "          tls: enable TLS\n"
           "         unix: use UNIX domain socket\n"
           "protocol.name: protocol and name to relay:\n"
           "                 - protocol \"irc\": name is the server to share "
           "(optional, if not given, the server name must be sent by client in "
           "command \"PASS\", with format: \"PASS server:password\")\n"
           "                 - protocol \"weechat\" (name is not used)\n"
           "\n"
           "The \"irc\" protocol allows any IRC client (including WeeChat "
           "itself) to connect on the port.\n"
           "The \"weechat\" protocol allows a remote interface to connect on "
           "the port, see the list here: https://weechat.org/about/interfaces/\n"
           "\n"
           "Without argument, this command opens buffer with list of relay "
           "clients.\n"
           "\n"
           "Examples:\n"
           "  irc proxy, for server \"libera\":\n"
           "    /relay add irc.libera 8000\n"
           "  irc proxy, for server \"libera\", with TLS:\n"
           "    /relay add tls.irc.libera 8001\n"
           "  irc proxy, for all servers (client will choose), with TLS:\n"
           "    /relay add tls.irc 8002\n"
           "  weechat protocol:\n"
           "    /relay add weechat 9000\n"
           "  weechat protocol with TLS:\n"
           "    /relay add tls.weechat 9001\n"
           "  weechat protocol with TLS, using only IPv4:\n"
           "    /relay add ipv4.tls.weechat 9001\n"
           "  weechat protocol with TLS, using only IPv6:\n"
           "    /relay add ipv6.tls.weechat 9001\n"
           "  weechat protocol with TLS, using IPv4 + IPv6:\n"
           "    /relay add ipv4.ipv6.tls.weechat 9001\n"
           "  weechat protocol over UNIX domain socket:\n"
           "    /relay add unix.weechat ${weechat_runtime_dir}/relay_socket"),
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
}
