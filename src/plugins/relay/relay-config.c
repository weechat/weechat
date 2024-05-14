/*
 * relay-config.c - relay configuration options (file relay.conf)
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <regex.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-buffer.h"
#include "relay-network.h"
#include "relay-remote.h"
#include "relay-server.h"
#include "irc/relay-irc.h"


struct t_config_file *relay_config_file = NULL;

/* sections */

struct t_config_section *relay_config_section_look = NULL;
struct t_config_section *relay_config_section_color = NULL;
struct t_config_section *relay_config_section_network = NULL;
struct t_config_section *relay_config_section_irc = NULL;
struct t_config_section *relay_config_section_api = NULL;
struct t_config_section *relay_config_section_port = NULL;
struct t_config_section *relay_config_section_path = NULL;
struct t_config_section *relay_config_section_remote = NULL;

/* relay config, look section */

struct t_config_option *relay_config_look_auto_open_buffer = NULL;
struct t_config_option *relay_config_look_display_clients = NULL;
struct t_config_option *relay_config_look_raw_messages = NULL;

/* relay config, color section */

struct t_config_option *relay_config_color_client = NULL;
struct t_config_option *relay_config_color_status[RELAY_NUM_STATUS] = {
    NULL, NULL, NULL, NULL, NULL,
};
struct t_config_option *relay_config_color_text = NULL;
struct t_config_option *relay_config_color_text_bg = NULL;
struct t_config_option *relay_config_color_text_selected = NULL;

/* relay config, network section */

struct t_config_option *relay_config_network_allow_empty_password = NULL;
struct t_config_option *relay_config_network_allowed_ips = NULL;
struct t_config_option *relay_config_network_auth_timeout = NULL;
struct t_config_option *relay_config_network_bind_address = NULL;
struct t_config_option *relay_config_network_clients_purge_delay = NULL;
struct t_config_option *relay_config_network_commands = NULL;
struct t_config_option *relay_config_network_compression = NULL;
struct t_config_option *relay_config_network_ipv6 = NULL;
struct t_config_option *relay_config_network_max_clients = NULL;
struct t_config_option *relay_config_network_nonce_size = NULL;
struct t_config_option *relay_config_network_password = NULL;
struct t_config_option *relay_config_network_password_hash_algo = NULL;
struct t_config_option *relay_config_network_password_hash_iterations = NULL;
struct t_config_option *relay_config_network_time_window = NULL;
struct t_config_option *relay_config_network_tls_cert_key = NULL;
struct t_config_option *relay_config_network_tls_priorities = NULL;
struct t_config_option *relay_config_network_totp_secret = NULL;
struct t_config_option *relay_config_network_totp_window = NULL;
struct t_config_option *relay_config_network_websocket_allowed_origins = NULL;

/* relay config, irc section */

struct t_config_option *relay_config_irc_backlog_max_minutes = NULL;
struct t_config_option *relay_config_irc_backlog_max_number = NULL;
struct t_config_option *relay_config_irc_backlog_since_last_disconnect = NULL;
struct t_config_option *relay_config_irc_backlog_since_last_message = NULL;
struct t_config_option *relay_config_irc_backlog_tags = NULL;
struct t_config_option *relay_config_irc_backlog_time_format = NULL;

/* relay config, api section */

struct t_config_option *relay_config_api_remote_get_lines = NULL;

/* other */

int relay_config_auto_open_buffer[RELAY_NUM_PROTOCOLS];
int relay_config_display_clients[RELAY_NUM_PROTOCOLS];
regex_t *relay_config_regex_allowed_ips = NULL;
regex_t *relay_config_regex_websocket_allowed_origins = NULL;
struct t_hashtable *relay_config_hashtable_irc_backlog_tags = NULL;
char **relay_config_network_password_hash_algo_list = NULL;


/*
 * Callback for changes on option "relay.look.auto_open_buffer".
 */

void
relay_config_change_auto_open_buffer_cb (const void *pointer, void *data,
                                         struct t_config_option *option)
{
    const char *auto_open_buffer;
    const char *value_on = "irc,weechat", *value_off = "";
    char **items;
    int i, num_items, protocol;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    auto_open_buffer = weechat_config_string (relay_config_look_auto_open_buffer);

    /* old option was a boolean, use these values for compatibility */
    if (strcmp (auto_open_buffer, "on") == 0)
        auto_open_buffer = value_on;
    else if (strcmp (auto_open_buffer, "off") == 0)
        auto_open_buffer = value_off;

    for (i = 0; i < RELAY_NUM_PROTOCOLS; i++)
    {
        relay_config_auto_open_buffer[i] = 0;
    }

    if (auto_open_buffer[0])
    {
        items = weechat_string_split (auto_open_buffer, ",", NULL, 0, 0,
                                      &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                protocol = relay_protocol_search (items[i]);
                if (protocol >= 0)
                    relay_config_auto_open_buffer[protocol] = 1;
            }
            weechat_string_free_split (items);
        }
    }
}

/*
 * Callback for changes on option "relay.look.display_clients".
 */

void
relay_config_change_display_clients_cb (const void *pointer, void *data,
                                        struct t_config_option *option)
{
    const char *display_clients;
    char **items;
    int i, num_items, protocol;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    display_clients = weechat_config_string (relay_config_look_display_clients);

    for (i = 0; i < RELAY_NUM_PROTOCOLS; i++)
    {
        relay_config_display_clients[i] = 0;
    }

    if (display_clients[0])
    {
        items = weechat_string_split (display_clients, ",", NULL, 0, 0,
                                      &num_items);
        if (items)
        {
            for (i = 0; i < num_items; i++)
            {
                protocol = relay_protocol_search (items[i]);
                if (protocol >= 0)
                    relay_config_display_clients[protocol] = 1;
            }
            weechat_string_free_split (items);
        }
    }
}

/*
 * Callback for changes on options that require a refresh of relay list.
 */

void
relay_config_refresh_cb (const void *pointer, void *data,
                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (relay_buffer)
        relay_buffer_refresh (NULL);
}

/*
 * Callback for changes on option "relay.network.allowed_ips".
 */

void
relay_config_change_network_allowed_ips (const void *pointer, void *data,
                                         struct t_config_option *option)
{
    const char *allowed_ips;

    /* make C compiler happy */
    (void) pointer;
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
 * Callback for changes on option "relay.network.password_hash_algo".
 */

void
relay_config_change_network_password_hash_algo (const void *pointer,
                                                void *data,
                                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (relay_config_network_password_hash_algo_list)
    {
        weechat_string_free_split (relay_config_network_password_hash_algo_list);
        relay_config_network_password_hash_algo_list = NULL;
    }

    relay_config_network_password_hash_algo_list = weechat_string_split (
        weechat_config_string (relay_config_network_password_hash_algo),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        NULL);
}

/*
 * Callback for changes on option "relay.network.bind_address".
 */

void
relay_config_change_network_bind_address_cb (const void *pointer, void *data,
                                             struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
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
relay_config_change_network_ipv6_cb (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    for (ptr_server = relay_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        relay_server_get_protocol_args (ptr_server->protocol_string,
                                        &ptr_server->ipv4, &ptr_server->ipv6,
                                        NULL, &ptr_server->unix_socket, NULL, NULL);
        relay_server_close_socket (ptr_server);
        relay_server_create_socket (ptr_server);
    }
}

/*
 * Callback for changes on option "relay.network.tls_cert_key".
 */

void
relay_config_change_network_tls_cert_key (const void *pointer, void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (relay_network_init_ok)
        relay_network_set_tls_cert_key (1);
}

/*
 * Checks if option "relay.network.totp_secret" is valid.
 *
 * Returns:
 *   1: value is valid
 *   0: value is not valid
 */

int
relay_config_check_network_totp_secret (const void *pointer, void *data,
                                        struct t_config_option *option,
                                        const char *value)
{
    char *totp_secret, *secret;
    int rc, length;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    totp_secret = NULL;
    secret = NULL;

    totp_secret = weechat_string_eval_expression (value, NULL, NULL, NULL);
    if (totp_secret && totp_secret[0])
    {
        secret = malloc (strlen (totp_secret) + 1);
        if (!secret)
            goto error;
        length = weechat_string_base_decode ("32", totp_secret, secret);
        if (length < 0)
            goto error;
    }

    rc = 1;
    goto end;

error:
    rc = 0;
    weechat_printf (NULL,
                    _("%s%s: invalid value for option "
                      "\"relay.network.totp_secret\"; it must be a valid "
                      "string encoded in base32 "
                      "(only letters and digits from 2 to 7)"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME);

end:
    free (totp_secret);
    free (secret);
    return rc;
}

/*
 * Checks if option "relay.network.tls_priorities" is valid.
 *
 * Returns:
 *   1: value is valid
 *   0: value is not valid
 */

int
relay_config_check_network_tls_priorities (const void *pointer, void *data,
                                           struct t_config_option *option,
                                           const char *value)
{
    gnutls_priority_t priority_cache;
    const char *pos_error;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    pos_error = value;

    if (value && value[0])
    {
        rc = gnutls_priority_init (&priority_cache, value, &pos_error);
        if (rc == GNUTLS_E_SUCCESS)
        {
            gnutls_priority_deinit (priority_cache);
            return 1;
        }
    }

    weechat_printf (NULL,
                    _("%s%s: invalid priorities string, error "
                      "at this position in string: \"%s\""),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                    (pos_error) ? pos_error : value);

    return 0;
}

/*
 * Callback for changes on option "relay.network.tls_priorities".
 */

void
relay_config_change_network_tls_priorities (const void *pointer, void *data,
                                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (relay_network_init_ok && relay_gnutls_priority_cache)
    {
        gnutls_priority_deinit (*relay_gnutls_priority_cache);
        relay_network_set_priority ();
    }
}

/*
 * Callback for changes on option "relay.network.websocket_allowed_origins".
 */

void
relay_config_change_network_websocket_allowed_origins (const void *pointer, void *data,
                                                       struct t_config_option *option)
{
    const char *allowed_origins;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (relay_config_regex_websocket_allowed_origins)
    {
        regfree (relay_config_regex_websocket_allowed_origins);
        free (relay_config_regex_websocket_allowed_origins);
        relay_config_regex_websocket_allowed_origins = NULL;
    }

    allowed_origins = weechat_config_string (relay_config_network_websocket_allowed_origins);
    if (allowed_origins && allowed_origins[0])
    {
        relay_config_regex_websocket_allowed_origins = malloc (sizeof (*relay_config_regex_websocket_allowed_origins));
        if (relay_config_regex_websocket_allowed_origins)
        {
            if (weechat_string_regcomp (relay_config_regex_websocket_allowed_origins,
                                        allowed_origins,
                                        REG_EXTENDED | REG_ICASE) != 0)
            {
                free (relay_config_regex_websocket_allowed_origins);
                relay_config_regex_websocket_allowed_origins = NULL;
            }
        }
    }
}

/*
 * Checks if IRC backlog tags are valid.
 *
 * Returns:
 *   1: IRC backlog tags are valid
 *   0: IRC backlog tags are not valid
 */

int
relay_config_check_irc_backlog_tags (const void *pointer, void *data,
                                     struct t_config_option *option,
                                     const char *value)
{
    char **tags;
    int num_tags, i, rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    rc = 1;

    /* "*" means all tags */
    if (strcmp (value, "*") == 0)
        return rc;

    /* split tags and check them */
    tags = weechat_string_split (value, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &num_tags);
    if (tags)
    {
        for (i = 0; i < num_tags; i++)
        {
            if (relay_irc_search_backlog_commands_tags (tags[i]) < 0)
            {
                rc = 0;
                break;
            }
        }
        weechat_string_free_split (tags);
    }

    return rc;
}

/*
 * Callback for changes on option "relay.irc.backlog_tags".
 */

void
relay_config_change_irc_backlog_tags (const void *pointer, void *data,
                                      struct t_config_option *option)
{
    char **items;
    int num_items, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!relay_config_hashtable_irc_backlog_tags)
    {
        relay_config_hashtable_irc_backlog_tags = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
    }
    else
        weechat_hashtable_remove_all (relay_config_hashtable_irc_backlog_tags);

    items = weechat_string_split (
        weechat_config_string (relay_config_irc_backlog_tags),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            weechat_hashtable_set (relay_config_hashtable_irc_backlog_tags,
                                   items[i],
                                   NULL);
        }
        weechat_string_free_split (items);
    }
}

/*
 * Checks if a port is valid.
 *
 * Returns:
 *   1: port is valid
 *   0: port is not valid
 */

int
relay_config_check_port_cb (const void *pointer, void *data,
                            struct t_config_option *option,
                            const char *value)
{
    char *error;
    long port;
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
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
 * Checks if a UNIX path is too long or empty.
 *
 * Returns:
 *   1: path is valid
 *   0: path is empty or too long
 */

int
relay_config_check_path_length (const char *path)
{
    struct sockaddr_un addr;
    size_t length, max_length;

    length = strlen (path);
    if (length == 0)
    {
        weechat_printf (NULL, _("%s%s: error: path is empty"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        return 0;
    }

    max_length = sizeof (addr.sun_path);
    if (length + 1 > max_length)
    {
        weechat_printf (NULL,
                        _("%s%s: error: path \"%s\" too long (length: %d; max: %d)"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME, path,
                        length, max_length);
        return 0;
    }

    return 1;
}

/*
 * Checks if a UNIX path is available: it is available if not existing, or
 * if a file of type socket already exists.
 *
 * Returns:
 *    0: path is available
 *   -1: path already exists and is not a socket
 *   -2: invalid path
 */

int
relay_config_check_path_available (const char *path)
{
    struct stat buf;
    int rc;

    rc = stat (path, &buf);

    /* OK if an existing file is a socket */
    if ((rc == 0) && S_ISSOCK(buf.st_mode))
        return 0;

    /* error if an existing file is NOT a socket */
    if (rc == 0)
        return -1;

    /* OK if the file does not exist */
    if (errno == ENOENT)
        return 0;

    /* on any other error, the path it considered as not available */
    return -2;
}

/*
 * Checks if a path is valid.
 *
 * Returns:
 *   1: path is valid
 *   0: path is not valid
 */

int
relay_config_check_path_cb (const void *pointer, void *data,
                            struct t_config_option *option,
                            const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!relay_config_check_path_length (value))
        return 0;

    if (relay_server_search_path (value))
    {
        weechat_printf (NULL, _("%s%s: error: path \"%s\" is already used"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME, value);
        return 0;
    }

    return 1;
}

/*
 * Callback for changes on options in section "path".
 */

void
relay_config_change_path_cb (const void *pointer, void *data,
                             struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_server)
    {
        relay_server_update_path (ptr_server,
                                  (const char *)weechat_config_option_get_pointer (option, "value"));
    }
}

/*
 * Callback called when an option is deleted in section "path".
 */

void
relay_config_delete_path_cb (const void *pointer, void *data,
                             struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    relay_server_free (ptr_server);
}

/*
 * Callback for changes on options in section "port".
 */

void
relay_config_change_port_cb (const void *pointer, void *data,
                             struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
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
relay_config_delete_port_cb (const void *pointer, void *data,
                             struct t_config_option *option)
{
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_server = relay_server_search (weechat_config_option_get_pointer (option, "name"));
    relay_server_free (ptr_server);
}

/*
 * Callback called when an option is created in section "port" or "path".
 */

int
relay_config_create_option_port_path (const void *pointer, void *data,
                                      struct t_config_file *config_file,
                                      struct t_config_section *section,
                                      const char *option_name,
                                      const char *value)
{
    int rc, protocol_number, ipv4, ipv6, tls, unix_socket;
    char *error, *protocol, *protocol_args;
    long port;
    struct t_relay_server *ptr_server;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    protocol_number = -1;
    port = -1;

    relay_server_get_protocol_args (option_name, &ipv4, &ipv6, &tls,
                                    &unix_socket, &protocol, &protocol_args);

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
        if (unix_socket)
        {
            ptr_server = relay_server_search_path (value);
        }
        else
        {
            error = NULL;
            port = strtol (value, &error, 10);
            ptr_server = relay_server_search_port ((int)port);
        }
        if (ptr_server)
        {
            if (unix_socket)
            {
                weechat_printf (NULL,
                                _("%s%s: error: path \"%s\" is already used"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                value);
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: error: port \"%d\" is already used"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                                (int)port);
            }
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
    }

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        if (relay_server_new (option_name, protocol_number, protocol_args,
                              port, value, ipv4, ipv6, tls, unix_socket))
        {
            /* create configuration option */
            if (unix_socket)
            {
                weechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("path to a socket file "
                      "(path is evaluated, see function string_eval_path_home "
                      "in plugin API reference)"),
                    NULL, 0, 0, "", value, 0,
                    &relay_config_check_path_cb, NULL, NULL,
                    &relay_config_change_path_cb, NULL, NULL,
                    &relay_config_delete_path_cb, NULL, NULL);
            }
            else
            {
                weechat_config_new_option (
                    config_file, section,
                    option_name, "integer",
                    _("port for relay"),
                    NULL, 0, 65535, "", value, 0,
                    &relay_config_check_port_cb, NULL, NULL,
                    &relay_config_change_port_cb, NULL, NULL,
                    &relay_config_delete_port_cb, NULL, NULL);
            }
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
        else
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    }

    free (protocol);
    free (protocol_args);

    return rc;
}

/*
 * Gets remote pointer with name of option.
 */

struct t_relay_remote *
relay_config_get_remote_from_option_name (const char *name)
{
    struct t_relay_remote *ptr_remote;
    char *pos_option, *remote_name;

    ptr_remote = NULL;

    if (name)
    {
        pos_option = strrchr (name, '.');
        if (pos_option)
        {
            remote_name = weechat_strndup (name, pos_option - name);
            if (remote_name)
            {
                ptr_remote = relay_remote_search (remote_name);
                free (remote_name);
            }
        }
    }

    return ptr_remote;
}

/*
 * Callback called to check a server option when it is modified.
 */

int
relay_config_remote_url_check_value_cb (const void *pointer, void *data,
                                        struct t_config_option *option,
                                        const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    return relay_remote_url_valid (value);
}

/*
 * Callback called when a remote URL option is modified.
 */

void
relay_config_remote_url_change_cb (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    struct t_relay_remote *ptr_remote;
    char *name;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    name = weechat_config_option_get_pointer (option, "name");
    ptr_remote = relay_config_get_remote_from_option_name (name);
    if (ptr_remote)
        relay_remote_set_url (ptr_remote, weechat_config_string (option));
}

/*
 * Creates an option for a remote.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
relay_config_create_remote_option (const char *remote_name, int index_option,
                                   const char *value)
{
    struct t_config_option *ptr_option;
    int length;
    char *option_name;

    ptr_option = NULL;

    length = strlen (remote_name) + 1 +
        strlen (relay_remote_option_string[index_option]) + 1;
    option_name = malloc (length);
    if (!option_name)
        return NULL;

    snprintf (option_name, length, "%s.%s",
              remote_name, relay_remote_option_string[index_option]);

    switch (index_option)
    {
        case RELAY_REMOTE_OPTION_URL:
            ptr_option = weechat_config_new_option (
                relay_config_file, relay_config_section_remote,
                option_name, "string",
                N_("remote relay URL with optional port (default is 9000), "
                   "examples: https://example.com:9000 or http://example.com:9000 "
                   "(plain-text connection, not recommended)"),
                NULL, 0, 0, value, NULL, 0,
                &relay_config_remote_url_check_value_cb, NULL, NULL,
                &relay_config_remote_url_change_cb, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case RELAY_REMOTE_OPTION_AUTOCONNECT:
            ptr_option = weechat_config_new_option (
                relay_config_file, relay_config_section_remote,
                option_name, "boolean",
                N_("auto-connect to the remote relay"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case RELAY_REMOTE_OPTION_PROXY:
            ptr_option = weechat_config_new_option (
                relay_config_file, relay_config_section_remote,
                option_name, "string",
                N_("name of proxy used for this remote relay (optional, proxy "
                   "must be defined with command /proxy)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case RELAY_REMOTE_OPTION_TLS_VERIFY:
            ptr_option = weechat_config_new_option (
                relay_config_file, relay_config_section_remote,
                option_name, "boolean",
                N_("check that the TLS connection is fully trusted"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case RELAY_REMOTE_OPTION_PASSWORD:
            ptr_option = weechat_config_new_option (
                relay_config_file, relay_config_section_remote,
                option_name, "string",
                N_("password for remote relay "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case RELAY_REMOTE_OPTION_TOTP_SECRET:
            ptr_option = weechat_config_new_option (
                relay_config_file, relay_config_section_remote,
                option_name, "string",
                N_("TOTP secret, encoded in base32 "
                   "(note: content is evaluated, see /help eval)"),
                NULL, 0, 0, value, NULL, 0,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            break;
        case RELAY_REMOTE_NUM_OPTIONS:
            break;
    }

    free (option_name);

    return ptr_option;
}

/*
 * Creates option for a temporary remote (when reading configuration file).
 */

void
relay_config_create_option_temp (struct t_relay_remote *temp_remote,
                                 int index_option, const char *value)
{
    struct t_config_option *new_option;

    new_option = relay_config_create_remote_option (temp_remote->name,
                                                    index_option, value);
    if (new_option
        && (index_option >= 0) && (index_option < RELAY_REMOTE_NUM_OPTIONS))
    {
        temp_remote->options[index_option] = new_option;
    }
}

/*
 * Uses temporary remotes (created by reading configuration file).
 */

void
relay_config_use_temp_remotes ()
{
    struct t_relay_remote *ptr_temp_remote, *next_temp_remote;
    int i, num_options_ok;

    for (ptr_temp_remote = relay_remotes_temp; ptr_temp_remote;
         ptr_temp_remote = ptr_temp_remote->next_remote)
    {
        num_options_ok = 0;
        for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
        {
            if (!ptr_temp_remote->options[i])
            {
                ptr_temp_remote->options[i] =
                    relay_config_create_remote_option (
                        ptr_temp_remote->name,
                        i,
                        relay_remote_option_default[i]);
            }
            if (ptr_temp_remote->options[i])
                num_options_ok++;
        }

        if (num_options_ok == RELAY_REMOTE_NUM_OPTIONS)
        {
            relay_remote_new_with_options (ptr_temp_remote->name,
                                           ptr_temp_remote->options);
        }
        else
        {
            for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
            {
                if (ptr_temp_remote->options[i])
                {
                    weechat_config_option_free (ptr_temp_remote->options[i]);
                    ptr_temp_remote->options[i] = NULL;
                }
            }
        }
    }

    /* free all temporary remotes */
    while (relay_remotes_temp)
    {
        next_temp_remote = relay_remotes_temp->next_remote;

        free (relay_remotes_temp->name);
        free (relay_remotes_temp);

        relay_remotes_temp = next_temp_remote;
    }
    last_relay_remote_temp = NULL;
}

/*
 * Reads a remote option in relay configuration file.
 */

int
relay_config_remote_read_cb (const void *pointer, void *data,
                             struct t_config_file *config_file,
                             struct t_config_section *section,
                             const char *option_name, const char *value)
{
    char *pos_option, *remote_name;
    struct t_relay_remote *ptr_temp_remote;
    int index_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (!option_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option = strchr (option_name, '.');
    if (!pos_option)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    remote_name = weechat_strndup (option_name, pos_option - option_name);
    if (!remote_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option++;

    /* search temporary remote */
    for (ptr_temp_remote = relay_remotes_temp; ptr_temp_remote;
         ptr_temp_remote = ptr_temp_remote->next_remote)
    {
        if (strcmp (ptr_temp_remote->name, remote_name) == 0)
            break;
    }
    if (!ptr_temp_remote)
    {
        /* create new temporary remote */
        ptr_temp_remote = relay_remote_alloc (remote_name);
        if (ptr_temp_remote)
            relay_remote_add (ptr_temp_remote, &relay_remotes_temp, &last_relay_remote_temp);
    }

    if (ptr_temp_remote)
    {
        index_option = relay_remote_search_option (pos_option);
        if (index_option >= 0)
        {
            relay_config_create_option_temp (ptr_temp_remote, index_option,
                                             value);
        }
        else
        {
            weechat_printf (NULL,
                            _("%sWarning: unknown option for section \"%s\": "
                              "%s (value: \"%s\")"),
                            weechat_prefix ("error"),
                            RELAY_CONFIG_SECTION_REMOTE,
                            option_name, value);
        }
    }

    free (remote_name);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Reloads relay configuration file.
 */

int
relay_config_reload (const void *pointer, void *data,
                     struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    weechat_config_section_free_options (relay_config_section_port);
    relay_server_free_all ();

    return weechat_config_reload (config_file);
}

/*
 * Updates options in configuration file while reading the file.
 */

struct t_hashtable *
relay_config_update_cb (const void *pointer, void *data,
                        struct t_config_file *config_file,
                        int version_read,
                        struct t_hashtable *data_read)
{
    const char *ptr_section, *ptr_option;
    char *new_option, *pos;
    int changes;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;

    /* nothing to do if the config file is already up-to-date */
    if (version_read >= RELAY_CONFIG_VERSION)
        return NULL;

    changes = 0;

    if (version_read < 2)
    {
        /*
         * changes in v2 (WeeChat 4.0.0):
         *   - options "ssl*" renamed to "tls*"
         *   - protocol "ssl" renamed to "tls" in port/path sections
         */
        ptr_section = weechat_hashtable_get (data_read, "section");
        ptr_option = weechat_hashtable_get (data_read, "option");
        if (ptr_section
            && ptr_option
            && (strcmp (ptr_section, "network") == 0))
        {
            if (strncmp (ptr_option, "ssl", 3) == 0)
            {
                new_option = strdup (ptr_option);
                if (new_option)
                {
                    memcpy (new_option, "tls", 3);
                    weechat_printf (
                        NULL,
                        _("Relay option renamed: \"relay.network.%s\" => "
                          "\"relay.network.%s\""),
                        ptr_option, new_option);
                    weechat_hashtable_set (data_read, "option", new_option);
                    changes++;
                    free (new_option);
                }
            }
        }
        else if (ptr_section
                 && ptr_option
                 && ((strcmp (ptr_section, "port") == 0)
                     || (strcmp (ptr_section, "path") == 0)))
        {
            new_option = strdup (ptr_option);
            if (new_option)
            {
                pos = new_option;
                while (1)
                {
                    if (strncmp (pos, "ipv4.", 5) == 0)
                    {
                        pos += 5;
                    }
                    else if (strncmp (pos, "ipv6.", 5) == 0)
                    {
                        pos += 5;
                    }
                    else if (strncmp (pos, "ssl.", 4) == 0)
                    {
                        memcpy (pos, "tls", 3);
                        pos += 4;
                    }
                    else if (strncmp (pos, "unix.", 5) == 0)
                    {
                        pos += 5;
                    }
                    else
                        break;
                }
                if (strcmp (ptr_option, new_option) != 0)
                {
                    weechat_printf (
                        NULL,
                        _("Relay option renamed: "
                          "\"relay.%s.%s\" => \"relay.%s.%s\""),
                        ptr_section, ptr_option,
                        ptr_section, new_option);
                    weechat_hashtable_set (data_read, "option", new_option);
                    changes++;
                }
                free (new_option);
            }
        }
    }

    return (changes) ? data_read : NULL;
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
    relay_config_file = weechat_config_new (RELAY_CONFIG_PRIO_NAME,
                                            &relay_config_reload, NULL, NULL);
    if (!relay_config_file)
        return 0;

    if (!weechat_config_set_version (relay_config_file, RELAY_CONFIG_VERSION,
                                     &relay_config_update_cb, NULL, NULL))
    {
        weechat_config_free (relay_config_file);
        relay_config_file = NULL;
        return 0;
    }

    /* section look */
    relay_config_section_look = weechat_config_new_section (
        relay_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (relay_config_section_look)
    {
        relay_config_look_auto_open_buffer = weechat_config_new_option (
            relay_config_file, relay_config_section_look,
            "auto_open_buffer", "string",
            N_("auto open relay buffer when a new client is connecting "
               "using one of these protocols (comma-separated list); "
               "allowed protocols: \"irc\", \"weechat\", \"api\""),
            NULL, 0, 0, "irc,weechat", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_auto_open_buffer_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_look_display_clients = weechat_config_new_option (
            relay_config_file, relay_config_section_look,
            "display_clients", "string",
            N_("display messages when clients connect/disconnect from relay "
               "using one of these protocols (comma-separated list); "
               "allowed protocols: \"irc\", \"weechat\", \"api\""),
            NULL, 0, 0, "irc,weechat", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_display_clients_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_look_raw_messages = weechat_config_new_option (
            relay_config_file, relay_config_section_look,
            "raw_messages", "integer",
            N_("number of raw messages to save in memory when raw data buffer "
               "is closed (messages will be displayed when opening raw data "
               "buffer)"),
            NULL, 0, 65535, "256", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* section color */
    relay_config_section_color = weechat_config_new_section (
        relay_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (relay_config_section_color)
    {
        relay_config_color_client = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "client", "color",
            N_("text color for client description"),
            NULL, 0, 0, "cyan", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_color_status[RELAY_STATUS_CONNECTED] = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "status_active", "color",
            N_("text color for \"connected\" status"),
            NULL, 0, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_status[RELAY_STATUS_AUTH_FAILED] = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "status_auth_failed", "color",
            N_("text color for \"authentication failed\" status"),
            NULL, 0, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_status[RELAY_STATUS_CONNECTING] = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "status_connecting", "color",
            N_("text color for \"connecting\" status"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_status[RELAY_STATUS_DISCONNECTED] = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "status_disconnected", "color",
            N_("text color for \"disconnected\" status"),
            NULL, 0, 0, "lightred", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_status[RELAY_STATUS_AUTHENTICATING] = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "status_authenticating", "color",
            N_("text color for \"authenticating\" status"),
            NULL, 0, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_text = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "text", "color",
            N_("text color in relay buffer"),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_text_bg = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "text_bg", "color",
            N_("background color in relay buffer"),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_color_text_selected = weechat_config_new_option (
            relay_config_file, relay_config_section_color,
            "text_selected", "color",
            N_("text color of selected line in relay buffer"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* section network */
    relay_config_section_network = weechat_config_new_section (
        relay_config_file, "network",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (relay_config_section_network)
    {
        relay_config_network_allow_empty_password = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "allow_empty_password", "boolean",
            N_("allow empty password in relay (it should be enabled only for "
               "tests or local network)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_allowed_ips = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "allowed_ips", "string",
            N_("POSIX extended regular expression with IPs allowed to use relay "
               "(case insensitive, use \"(?-i)\" at beginning to make it case "
               "sensitive), example: "
               "\"^(123\\.45\\.67\\.89|192\\.160\\..*)$\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_network_allowed_ips, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_auth_timeout = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "auth_timeout", "integer",
            N_("timeout (in seconds) for client authentication: connection is "
               "closed if the client is still not authenticated after this "
               "delay and the client status is set to \"authentication "
               "failed\" (0 = wait forever)"),
            NULL, 0, INT_MAX, "60", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_bind_address = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "bind_address", "string",
            N_("address for bind (if empty, connection is possible on all "
               "interfaces, use \"127.0.0.1\" to allow connections from "
               "local machine only with IPv4 and \"::ffff:127.0.0.1\" with IPv6)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_network_bind_address_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_clients_purge_delay = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "clients_purge_delay", "integer",
            N_("delay for purging disconnected clients (in minutes, 0 = purge "
               "clients immediately, -1 = never purge)"),
            NULL, -1, 60 * 24 * 30, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_commands = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "commands", "string",
            N_("comma-separated list of commands allowed/denied when input "
               "data (text or command) is received from a client (weechat "
               "and api protocols); "
               "\"*\" means any command, a name beginning with \"!\" is "
               "a negative value to prevent a command from being executed, "
               "wildcard \"*\" is allowed in names; this option should be set "
               "if the relay client is not safe (someone could use it to run "
               "commands); for example \"*,!exec,!quit\" allows any command "
               "except /exec and /quit"),
            NULL, 0, 0, "*,!quit", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_compression = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "compression", "integer",
            N_("compression of messages sent to clients with weechat and "
               "api protocols: 0 = disable compression, "
               "1 = low compression / fast ... 100 = best compression / slow; "
               "the value is a percentage converted to 1-9 for zlib and 1-19 "
               "for zstd; the default value is recommended, it offers a good "
               "compromise between compression and speed"),
            NULL, 0, 100, "20", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_ipv6 = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "ipv6", "boolean",
            N_("listen on IPv6 socket by default (in addition to IPv4 which is "
               "default); protocols IPv4 and IPv6 can be forced (individually "
               "or together) in the protocol name (see /help relay)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_network_ipv6_cb, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_max_clients = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "max_clients", "integer",
            N_("maximum number of clients connecting to a port (0 = no limit)"),
            NULL, 0, INT_MAX, "5", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_nonce_size = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "nonce_size", "integer",
            N_("size of nonce (in bytes), generated when a client connects; "
               "the client must use this nonce, concatenated to the client nonce "
               "and the password when hashing the password in the \"init\" "
               "command of the weechat protocol"),
            NULL, 8, 128, "16", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_password = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "password", "string",
            N_("password required by clients to access this relay (empty value "
               "means no password required, see option "
               "relay.network.allow_empty_password) (note: content is evaluated, "
               "see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_password_hash_algo = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "password_hash_algo", "string",
            N_("comma separated list of hash algorithms used for password "
               "authentication in weechat protocol, among these values: "
               "\"plain\" (password in plain text, not hashed), \"sha256\", "
               "\"sha512\", \"pbkdf2+sha256\", \"pbkdf2+sha512\"), \"*\" means "
               "all algorithms, a name beginning with \"!\" is a negative "
               "value to prevent an algorithm from being used, wildcard \"*\" "
               "is allowed in names (examples: \"*\", \"pbkdf2*\", "
               "\"*,!plain\")"),
            NULL, 0, 0, "*", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_network_password_hash_algo, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_password_hash_iterations = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "password_hash_iterations", "integer",
            N_("number of iterations asked to the client in weechat protocol "
               "when a hashed password with algorithm PBKDF2 is used for "
               "authentication; more iterations is better in term of security "
               "but is slower to compute; this number should not be too high "
               "if your CPU is slow"),
            NULL, 1, 1000000, "100000", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_time_window = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "time_window", "integer",
            N_("number of seconds to allow before and after the current time "
               "for the hash of time + password in api protocol"),
            NULL, 0, 256, "5", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_tls_cert_key = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "tls_cert_key", "string",
            N_("file with TLS certificate and private key (for serving clients "
               "with TLS) "
               "(path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "${weechat_config_dir}/tls/relay.pem", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_network_tls_cert_key, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_tls_priorities = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "tls_priorities", "string",
            N_("string with priorities for gnutls (for syntax, see "
               "documentation of function gnutls_priority_init in gnutls "
               "manual, common strings are: \"PERFORMANCE\", \"NORMAL\", "
               "\"SECURE128\", \"SECURE256\", \"EXPORT\", \"NONE\")"),
            NULL, 0, 0, "NORMAL", NULL, 0,
            &relay_config_check_network_tls_priorities, NULL, NULL,
            &relay_config_change_network_tls_priorities, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_totp_secret = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "totp_secret", "string",
            N_("secret for the generation of the Time-based One-Time Password "
               "(TOTP), encoded in base32 (only letters and digits from 2 to 7); "
               "it is used as second factor in weechat and api protocols, "
               "in addition to the password, which must not be empty "
               "(empty value means no TOTP is required) "
               "(note: content is evaluated, see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            &relay_config_check_network_totp_secret, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_network_totp_window = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "totp_window", "integer",
            N_("number of Time-based One-Time Passwords to accept before and "
               "after the current one: "
               "0 = accept only the current password, "
               "1 = accept one password before, the current, and one after, "
               "2 = accept two passwords before, the current, and two after, "
               "...; a high number reduces the security level "
               "(0 or 1 are recommended values)"),
            NULL, 0, 256, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_network_websocket_allowed_origins = weechat_config_new_option (
            relay_config_file, relay_config_section_network,
            "websocket_allowed_origins", "string",
            N_("POSIX extended regular expression with origins allowed in "
               "websockets (case insensitive, use \"(?-i)\" at beginning to "
               "make it case sensitive), example: "
               "\"^https?://(www\\.)?example\\.(com|org)\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &relay_config_change_network_websocket_allowed_origins, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* section irc */
    relay_config_section_irc = weechat_config_new_section (
        relay_config_file, "irc",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (relay_config_section_irc)
    {
        relay_config_irc_backlog_max_minutes = weechat_config_new_option (
            relay_config_file, relay_config_section_irc,
            "backlog_max_minutes", "integer",
            N_("maximum number of minutes in backlog per IRC channel "
               "(0 = unlimited, examples: 1440 = one day, 10080 = one week, "
               "43200 = one month, 525600 = one year)"),
            NULL, 0, INT_MAX, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_irc_backlog_max_number = weechat_config_new_option (
            relay_config_file, relay_config_section_irc,
            "backlog_max_number", "integer",
            N_("maximum number of lines in backlog per IRC channel "
               "(0 = unlimited)"),
            NULL, 0, INT_MAX, "1024", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_irc_backlog_since_last_disconnect = weechat_config_new_option (
            relay_config_file, relay_config_section_irc,
            "backlog_since_last_disconnect", "boolean",
            N_("display backlog starting from last client disconnect"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_irc_backlog_since_last_message = weechat_config_new_option (
            relay_config_file, relay_config_section_irc,
            "backlog_since_last_message", "boolean",
            N_("display backlog starting from your last message"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        relay_config_irc_backlog_tags = weechat_config_new_option (
            relay_config_file, relay_config_section_irc,
            "backlog_tags", "string",
            N_("comma-separated list of messages tags which are displayed in "
               "backlog per IRC channel (supported tags: \"irc_join\", "
               "\"irc_part\", \"irc_quit\", \"irc_nick\", \"irc_privmsg\"), "
               "\"*\" = all supported tags"),
            NULL, 0, 0, "irc_privmsg", NULL, 0,
            &relay_config_check_irc_backlog_tags, NULL, NULL,
            &relay_config_change_irc_backlog_tags, NULL, NULL,
            NULL, NULL, NULL);
        relay_config_irc_backlog_time_format = weechat_config_new_option (
            relay_config_file, relay_config_section_irc,
            "backlog_time_format", "string",
            N_("format for time in backlog messages (see man strftime for "
               "format) (not used if server capability \"server-time\" was "
               "enabled by client, because time is sent as irc tag); empty "
               "string = disable time in backlog messages"),
            NULL, 0, 0, "[%H:%M] ", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* section api */
    relay_config_section_api = weechat_config_new_section (
        relay_config_file, "api",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (relay_config_section_api)
    {
        relay_config_api_remote_get_lines = weechat_config_new_option (
            relay_config_file, relay_config_section_api,
            "remote_get_lines", "integer",
            N_("number of lines to retrieve on each buffer when connecting "
               "to a remote relay"),
            NULL, 0, INT_MAX, "1000", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* section port */
    relay_config_section_port = weechat_config_new_section (
        relay_config_file, "port",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &relay_config_create_option_port_path, NULL, NULL,
        NULL, NULL, NULL);

    /* section path */
    relay_config_section_path = weechat_config_new_section (
        relay_config_file, "path",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &relay_config_create_option_port_path, NULL, NULL,
        NULL, NULL, NULL);

    /* remote */
    relay_config_section_remote = weechat_config_new_section (
        relay_config_file,
        RELAY_CONFIG_SECTION_REMOTE,
        0, 0,
        &relay_config_remote_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    return 1;
}

/*
 * Reads relay configuration file.
 */

int
relay_config_read ()
{
    int rc;

    rc = weechat_config_read (relay_config_file);
    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        relay_config_change_auto_open_buffer_cb (NULL, NULL, NULL);
        relay_config_change_display_clients_cb (NULL, NULL, NULL);
        relay_config_change_network_allowed_ips (NULL, NULL, NULL);
        relay_config_change_network_password_hash_algo (NULL, NULL, NULL);
        relay_config_change_irc_backlog_tags (NULL, NULL, NULL);
        relay_config_use_temp_remotes ();
    }
    return rc;
}

/*
 * Writes relay configuration file.
 */

int
relay_config_write ()
{
    return weechat_config_write (relay_config_file);
}

/*
 * Frees relay configuration.
 */

void
relay_config_free ()
{
    weechat_config_free (relay_config_file);
    relay_config_file = NULL;

    if (relay_config_regex_allowed_ips)
    {
        regfree (relay_config_regex_allowed_ips);
        free (relay_config_regex_allowed_ips);
        relay_config_regex_allowed_ips = NULL;
    }

    if (relay_config_regex_websocket_allowed_origins)
    {
        regfree (relay_config_regex_websocket_allowed_origins);
        free (relay_config_regex_websocket_allowed_origins);
        relay_config_regex_websocket_allowed_origins = NULL;
    }

    if (relay_config_hashtable_irc_backlog_tags)
    {
        weechat_hashtable_free (relay_config_hashtable_irc_backlog_tags);
        relay_config_hashtable_irc_backlog_tags = NULL;
    }

    if (relay_config_network_password_hash_algo_list)
    {
        weechat_string_free_split (relay_config_network_password_hash_algo_list);
        relay_config_network_password_hash_algo_list = NULL;
    }
}
