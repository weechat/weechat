/*
 * relay-config.c - relay configuration options (file relay.conf)
 *
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

#include <stdlib.h>
#include <limits.h>
#include <regex.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-config.h"
#include "relay-client.h"
#include "relay-buffer.h"
#include "relay-network.h"
#include "relay-server.h"


struct t_config_file *relay_config_file = NULL;
struct t_config_section *relay_config_section_port = NULL;

/* relay config, look section */

struct t_config_option *relay_config_look_auto_open_buffer;
struct t_config_option *relay_config_look_raw_messages;

/* relay config, color section */

struct t_config_option *relay_config_color_client;
struct t_config_option *relay_config_color_text;
struct t_config_option *relay_config_color_text_bg;
struct t_config_option *relay_config_color_text_selected;
struct t_config_option *relay_config_color_status[RELAY_NUM_STATUS];

/* relay config, network section */

struct t_config_option *relay_config_network_allowed_ips;
struct t_config_option *relay_config_network_bind_address;
struct t_config_option *relay_config_network_compression_level;
struct t_config_option *relay_config_network_ipv6;
struct t_config_option *relay_config_network_max_clients;
struct t_config_option *relay_config_network_password;
struct t_config_option *relay_config_network_ssl_cert_key;

/* other */

regex_t *relay_config_regex_allowed_ips = NULL;


/*
 * Callback for changes on options that require a refresh of relay list.
 */

void
relay_config_refresh_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (relay_buffer)
        relay_buffer_refresh (NULL);
}

/*
 * Callback for changes on option "relay.network.allowed_ips".
 */

void
relay_config_change_network_allowed_ips (void *data,
                                         struct t_config_option *option)
{
    const char *allowed_ips;

    /* make C compiler happy */
    (void) data;
    (void) option;

    if (relay_config_regex_allowed_ips)
    {
        regfree (relay_config_regex_allowed_ips);
        free (relay_config_regex_allowed_ips);
        relay_config_regex_allowed_ips = NULL;
    }

    allowed_ips = weechat_config_string (relay_config_network_allowed_ips);
    if (allowed_ips && allowed_ips[0])
    {
        relay_config_regex_allowed_ips = malloc (sizeof (*relay_config_regex_allowed_ips));
        if (relay_config_regex_allowed_ips)
        {
            if (weechat_string_regcomp (relay_config_regex_allowed_ips,
                                        allowed_ips,
                                        REG_EXTENDED | REG_ICASE) != 0)
            {
                free (relay_config_regex_allowed_ips);
                relay_config_regex_allowed_ips = NULL;
            }
        }
    }
}

/*
 * Callback for changes on option "relay.network.bind_address".
 */

void
relay_config_change_network_bind_address_cb (void *data,
                                             struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) data;
    (void) option;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        relay_server_close_socket (ptr_server);
        relay_server_create_socket (ptr_server);
    }
}

/*
 * Callback for changes on option "relay.network.ipv6".
 */

void
relay_config_change_network_ipv6_cb (void *data, struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) data;
    (void) option;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        relay_server_get_protocol_args (ptr_server->protocol_string,
                                        &ptr_server->ipv4, &ptr_server->ipv6,
                                        NULL, NULL, NULL);
        relay_server_close_socket (ptr_server);
        relay_server_create_socket (ptr_server);
    }
}

/*
 * Callback for changes on option "relay.network.ssl_cert_key".
 */

void
relay_config_change_network_ssl_cert_key (void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    if (relay_network_init_ok)
        relay_network_set_ssl_cert_key (1);
}

/*
 * Checks if a port is valid.
 *
 * Returns:
 *   1: port is valid
 *   0: port is not valid
 */

int
relay_config_check_port_cb (void *data, struct t_config_option *option,
                            const char *value)
{
    char *error;
    long port;
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) data;
    (void) option;

    error = NULL;
    port = strtol (value, &error, 10);
    ptr_server = relay_server_search_port ((int)port);
    if (ptr_server)
    {
        weechat_printf (NULL, _("%s%s: error: port \"%d\" is already used"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME, (int)port);
        return 0;
    }

    return 1;
}

/*
 * Callback for changes on options in section "port".
 */

void
relay_config_change_port_cb (void *data, struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) data;

    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_server)
    {
        relay_server_update_port (ptr_server,
                                  *((int *)weechat_config_option_get_pointer (option, "value")));
    }
}

/*
 * Callback called when an option is deleted in section "port".
 */

void
relay_config_delete_port_cb (void *data, struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) data;

    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_server)
        relay_server_free (ptr_server);
}

/*
 * Callback called when an option is created in section "port".
 */

int
relay_config_create_option_port (void *data,
                                 struct t_config_file *config_file,
                                 struct t_config_section *section,
                                 const char *option_name,
                                 const char *value)
{
    int rc, protocol_number, ipv4, ipv6, ssl;
    char *error, *protocol, *protocol_args;
    long port;
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    protocol_number = -1;
    port = -1;

    relay_server_get_protocol_args (option_name, &ipv4, &ipv6, &ssl,
                                    &protocol, &protocol_args);

#ifndef HAVE_GNUTLS
    if (ssl)
    {
        weechat_printf (NULL,
                        _("%s%s: cannot use SSL because WeeChat was not built "
                          "with GnuTLS support"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    }
#endif

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        if (protocol)
            protocol_number = relay_protocol_search (protocol);

        if (protocol_number < 0)
        {
            weechat_printf (NULL, _("%s%s: error: unknown protocol \"%s\""),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME, protocol);
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }

        if ((protocol_number == RELAY_PROTOCOL_WEECHAT) && protocol_args)
        {
            weechat_printf (NULL, _("%s%s: error: name is not allowed for "
                                    "protocol \"%s\""),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME, protocol);
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
        else if ((protocol_number == RELAY_PROTOCOL_IRC) && !protocol_args)
        {
            weechat_printf (NULL, _("%s%s: error: name is not required for "
                                    "protocol \"%s\""),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME, protocol);
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
    }

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        if (weechat_config_search_option (config_file, section, option_name))
        {
            weechat_printf (NULL, _("%s%s: error: relay for \"%s\" already exists"),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME, option_name);
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
    }

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        error = NULL;
        port = strtol (value, &error, 10);
        ptr_server = relay_server_search_port ((int)port);
        if (ptr_server)
        {
            weechat_printf (NULL, _("%s%s: error: port \"%d\" is already used"),
                            weechat_prefix ("error"),
                            RELAY_PLUGIN_NAME, (int)port);
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
    }

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        if (relay_server_new (option_name, protocol_number, protocol_args,
                              port, ipv4, ipv6, ssl))
        {
            /* create configuration option */
            weechat_config_new_option (
                config_file, section,
                option_name, "integer", NULL,
                NULL, 0, 65535, "", value, 0,
                &relay_config_check_port_cb, NULL,
                &relay_config_change_port_cb, NULL,
                &relay_config_delete_port_cb, NULL);
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
        else
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    }

    if (protocol)
        free (protocol);
    if (protocol_args)
        free (protocol_args);

    return rc;
}

/*
 * Reloads relay configuration file.
 */

int
relay_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;

    return  weechat_config_reload (config_file);
}

/*
 * Initializes relay configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_config_init ()
{
    struct t_config_section *ptr_section;

    relay_config_file = weechat_config_new (RELAY_CONFIG_NAME,
                                            &relay_config_reload, NULL);
    if (!relay_config_file)
        return 0;

    ptr_section = weechat_config_new_section (relay_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }

    relay_config_look_auto_open_buffer = weechat_config_new_option (
        relay_config_file, ptr_section,
        "auto_open_buffer", "boolean",
        N_("auto open relay buffer when a new client is connecting"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    relay_config_look_raw_messages = weechat_config_new_option (
        relay_config_file, ptr_section,
        "raw_messages", "integer",
        N_("number of raw messages to save in memory when raw data buffer is "
           "closed (messages will be displayed when opening raw data buffer)"),
        NULL, 0, 65535, "256", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (relay_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }

    relay_config_color_client = weechat_config_new_option (
        relay_config_file, ptr_section,
        "client", "color",
        N_("text color for client description"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    relay_config_color_text = weechat_config_new_option (
        relay_config_file, ptr_section,
        "text", "color",
        N_("text color in relay buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_text_bg = weechat_config_new_option (
        relay_config_file, ptr_section,
        "text_bg", "color",
        N_("background color in relay buffer"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_text_selected = weechat_config_new_option (
        relay_config_file, ptr_section,
        "text_selected", "color",
        N_("text color of selected line in relay buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_CONNECTING] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_connecting", "color",
        N_("text color for \"connecting\" status"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_WAITING_AUTH] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_waiting_auth", "color",
        N_("text color for \"waiting authentication\" status"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_CONNECTED] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_active", "color",
        N_("text color for \"connected\" status"),
        NULL, 0, 0, "lightblue", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_AUTH_FAILED] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_auth_failed", "color",
        N_("text color for \"authentication failed\" status"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);
    relay_config_color_status[RELAY_STATUS_DISCONNECTED] = weechat_config_new_option (
        relay_config_file, ptr_section,
        "status_disconnected", "color",
        N_("text color for \"disconnected\" status"),
        NULL, 0, 0, "lightred", NULL, 0,
        NULL, NULL, &relay_config_refresh_cb, NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (relay_config_file, "network",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }

    relay_config_network_allowed_ips = weechat_config_new_option (
        relay_config_file, ptr_section,
        "allowed_ips", "string",
        N_("regular expression with IPs allowed to use relay (case insensitive, "
           "use \"(?-i)\" at beginning to make it case sensitive); if IPv6 is "
           "enabled and that connection is made using IPv4, it will be "
           "IPv4-mapped IPv6 address (like: \"::ffff:127.0.0.1\"), example: "
           "\"^((::ffff:)?123.45.67.89|192.160.*)$\""),
        NULL, 0, 0, "", NULL, 0, NULL, NULL,
        &relay_config_change_network_allowed_ips, NULL, NULL, NULL);
    relay_config_network_bind_address = weechat_config_new_option (
        relay_config_file, ptr_section,
        "bind_address", "string",
        N_("address for bind (if empty, connection is possible on all "
           "interfaces, use \"127.0.0.1\" to allow connections from "
            "local machine only)"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL,
        &relay_config_change_network_bind_address_cb, NULL, NULL, NULL);
    relay_config_network_compression_level = weechat_config_new_option (
        relay_config_file, ptr_section,
        "compression_level", "integer",
        N_("compression level for packets sent to client with WeeChat protocol "
           "(0 = disable compression, 1 = low compression ... 9 = best "
           "compression)"),
        NULL, 0, 9, "6", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    relay_config_network_ipv6 = weechat_config_new_option (
        relay_config_file, ptr_section,
        "ipv6", "boolean",
        N_("listen on IPv6 socket by default (in addition to IPv4 which is "
            "default); protocols IPv4 and IPv6 can be forced (individually or "
           "together) in the protocol name (see /help relay)"),
        NULL, 0, 0, "on", NULL, 0, NULL, NULL,
        &relay_config_change_network_ipv6_cb, NULL, NULL, NULL);
    relay_config_network_max_clients = weechat_config_new_option (
        relay_config_file, ptr_section,
        "max_clients", "integer",
        N_("maximum number of clients connecting to a port"),
        NULL, 1, 1024, "5", NULL, 0,
        NULL, NULL, NULL, NULL, NULL, NULL);
    relay_config_network_password = weechat_config_new_option (
        relay_config_file, ptr_section,
        "password", "string",
        N_("password required by clients to access this relay (empty value "
            "means no password required)"),
        NULL, 0, 0, "", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    relay_config_network_ssl_cert_key = weechat_config_new_option (
        relay_config_file, ptr_section,
        "ssl_cert_key", "string",
        N_("file with SSL certificate and private key (for serving clients "
           "with SSL)"),
        NULL, 0, 0, "%h/ssl/relay.pem", NULL, 0, NULL, NULL,
        &relay_config_change_network_ssl_cert_key, NULL, NULL, NULL);

    ptr_section = weechat_config_new_section (relay_config_file, "port",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &relay_config_create_option_port, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (relay_config_file);
        return 0;
    }

    relay_config_section_port = ptr_section;

    return 1;
}

/*
 * Reads relay configuration file.
 */

int
relay_config_read ()
{
    return weechat_config_read (relay_config_file);
}

/*
 * Writes relay configuration file.
 */

int
relay_config_write ()
{
    return weechat_config_write (relay_config_file);
}
