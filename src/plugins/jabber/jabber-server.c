/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* jabber-server.c: connection and I/O communication with Jabber server */


#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#endif

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#include <iksemel.h>

#include "../weechat-plugin.h"
#include "jabber.h"
#include "jabber-server.h"
#include "jabber-buddy.h"
#include "jabber-buffer.h"
#include "jabber-command.h"
#include "jabber-config.h"
#include "jabber-debug.h"
#include "jabber-muc.h"
#include "jabber-xmpp.h"


struct t_jabber_server *jabber_servers = NULL;
struct t_jabber_server *last_jabber_server = NULL;

/* current server when there is one buffer for all servers */
struct t_jabber_server *jabber_current_server = NULL;

struct t_jabber_message *jabber_recv_msgq = NULL;
struct t_jabber_message *jabber_msgq_last_msg = NULL;

ikstransport jabber_iks_transport =
{
    .abi_version = IKS_TRANSPORT_V1,
    .connect = NULL,
    .send = &jabber_server_iks_transport_send,
    .recv = &jabber_server_iks_transport_recv,
    .close = &jabber_server_iks_transport_close,
    .connect_async = &jabber_server_iks_transport_connect_async,
};

char *jabber_server_option_string[JABBER_SERVER_NUM_OPTIONS] =
{ "username", "server", "proxy", "ipv6", "tls", "sasl", "resource", "password",
  "local_alias", "autoconnect", "autoreconnect", "autoreconnect_delay",
  "local_hostname", "command", "command_delay", "autojoin", "autorejoin"
};

char *jabber_server_option_default[JABBER_SERVER_NUM_OPTIONS] =
{ "", "", "", "off", "off", "on", "", "", "", "off", "on", "10", "", "", "0",
  "", "off"
};


/*
 * jabber_server_valid: check if a server pointer exists
 *                      return 1 if server exists
 *                             0 if server is not found
 */

int
jabber_server_valid (struct t_jabber_server *server)
{
    struct t_jabber_server *ptr_server;
    
    if (!server)
        return 0;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server == server)
            return 1;
    }
    
    /* server not found */
    return 0;
}

/*
 * jabber_server_search_option: search a server option name
 *                              return index of option in array
 *                              "jabber_server_option_string", or -1 if
 *                              not found
 */

int
jabber_server_search_option (const char *option_name)
{
    int i;
    
    if (!option_name)
        return -1;
    
    for (i = 0; i < JABBER_SERVER_NUM_OPTIONS; i++)
    {
        if (weechat_strcasecmp (jabber_server_option_string[i],
                                option_name) == 0)
            return i;
    }
    
    /* server option not found */
    return -1;
}

/*
 * jabber_server_search: return pointer on a server with a name
 */

struct t_jabber_server *
jabber_server_search (const char *server_name)
{
    struct t_jabber_server *ptr_server;
    
    if (!server_name)
        return NULL;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (strcmp (ptr_server->name, server_name) == 0)
            return ptr_server;
    }
    
    /* server not found */
    return NULL;
}

/*
 * jabber_server_get_muc_count: return number of MUCs for server
 */

int
jabber_server_get_muc_count (struct t_jabber_server *server)
{
    int count;
    struct t_jabber_muc *ptr_muc;
    
    count = 0;
    for (ptr_muc = server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
    {
        if (ptr_muc->type == JABBER_MUC_TYPE_MUC)
            count++;
    }
    return count;
}

/*
 * jabber_server_get_pv_count: return number of pv for server
 */

int
jabber_server_get_pv_count (struct t_jabber_server *server)
{
    int count;
    struct t_jabber_muc *ptr_muc;
    
    count = 0;
    for (ptr_muc = server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
    {
        if (ptr_muc->type == JABBER_MUC_TYPE_PRIVATE)
            count++;
    }
    return count;
}

/*
 * jabber_server_get_name_without_port: get name of server without port
 *                                      (ends before first '/' if found)
 */

char *
jabber_server_get_name_without_port (const char *name)
{
    char *pos;
    
    if (!name)
        return NULL;
    
    pos = strchr (name, '/');
    if (pos && (pos != name))
        return weechat_strndup (name, pos - name);
    
    return strdup (name);
}

/*
 * jabber_server_get_local_name: get local alias for server (if defined),
 *                               otherwise return username
 */

const char *
jabber_server_get_local_name (struct t_jabber_server *server)
{
    const char *local_alias;
    
    local_alias = JABBER_SERVER_OPTION_STRING(server,
                                              JABBER_SERVER_OPTION_LOCAL_ALIAS);
    if (local_alias && local_alias[0])
        return local_alias;
    
    /* fallback to username */
    return JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_USERNAME);
}

/*
 * jabber_server_set_server: set server address
 */

void
jabber_server_set_server (struct t_jabber_server *server,
                          const char *address)
{
    char *pos, *error;
    long number;
    
    /* free data */
    if (server->address)
    {
        free (server->address);
        server->address = NULL;
    }
    server->port = JABBER_SERVER_DEFAULT_PORT;
    
    if (address && address[0])
    {
        pos = strchr (address, '/');
        if (pos && (pos > address))
        {
            server->address = weechat_strndup (address, pos - address);
            pos++;
            error = NULL;
            number = strtol (pos, &error, 10);
            if (error && !error[0])
                server->port = number;
        }
        else
            server->address = strdup (address);
    }
}

/*
 * jabber_server_buffer_set_highlight_words: set highlight words for buffer
 *                                           with all servers
 */

void
jabber_server_buffer_set_highlight_words (struct t_gui_buffer *buffer)
{
    struct t_jabber_server *ptr_server;
    int length;
    const char *local_name;
    char *words;
    
    length = 0;
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (ptr_server->is_connected)
        {
            local_name = jabber_server_get_local_name (ptr_server);
            if (local_name && local_name[0])
                length += strlen (local_name) + 1;
        }
    }
    words = malloc (length + 1);
    if (words)
    {
        words[0] = '\0';
        for (ptr_server = jabber_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->is_connected)
            {
                local_name = jabber_server_get_local_name (ptr_server);
                if (local_name && local_name[0])
                {
                    if (words[0])
                        strcat (words, ",");
                    strcat (words, local_name);
                }
            }
        }
        weechat_buffer_set (buffer, "highlight_words", words);
        free (words);
    }
}

/*
 * jabber_server_alloc: allocate a new server and add it to the servers queue
 */

struct t_jabber_server *
jabber_server_alloc (const char *name)
{
    struct t_jabber_server *new_server;
    int i, length;
    char *option_name;
    
    if (jabber_server_search (name))
        return NULL;
    
    /* alloc memory for new server */
    new_server = malloc (sizeof (*new_server));
    if (!new_server)
    {
        weechat_printf (NULL,
                        _("%s%s: error when allocating new server"),
                        weechat_prefix ("error"), JABBER_PLUGIN_NAME);
        return NULL;
    }
    
    /* add new server to queue */
    new_server->prev_server = last_jabber_server;
    new_server->next_server = NULL;
    if (jabber_servers)
        last_jabber_server->next_server = new_server;
    else
        jabber_servers = new_server;
    last_jabber_server = new_server;
    
    /* set name */
    new_server->name = strdup (name);
    
    /* internal vars */
    new_server->temp_server = 0;
    new_server->reloading_from_config = 0;
    new_server->reloaded_from_config = 0;
    new_server->address = NULL;
    new_server->port = JABBER_SERVER_DEFAULT_PORT;
    new_server->current_ip = NULL;
    new_server->sock = -1;
    new_server->iks_parser = NULL;
    new_server->iks_id_string = NULL;
    new_server->iks_id = NULL;
    new_server->iks_server_name = NULL;
    new_server->iks_password = NULL;
    new_server->iks_filter = NULL;
    new_server->iks_roster = NULL;
    new_server->iks_features = 0;
    new_server->iks_authorized = 0;
    new_server->hook_connect = NULL;
    new_server->hook_fd = NULL;
    new_server->is_connected = 0;
    new_server->tls_connected = 0;
    new_server->reconnect_start = 0;
    new_server->command_time = 0;
    new_server->reconnect_join = 0;
    new_server->disable_autojoin = 0;
    new_server->is_away = 0;
    new_server->away_message = NULL;
    new_server->away_time = 0;
    new_server->lag = 0;
    new_server->lag_check_time.tv_sec = 0;
    new_server->lag_check_time.tv_usec = 0;
    new_server->lag_next_check = time (NULL) +
        weechat_config_integer (jabber_config_network_lag_check);
    new_server->buffer = NULL;
    new_server->buffer_as_string = NULL;
    new_server->buddies_count = 0;
    new_server->buddies = NULL;
    new_server->last_buddy = NULL;
    new_server->mucs = NULL;
    new_server->last_muc = NULL;
    
    /* create options with null value */
    for (i = 0; i < JABBER_SERVER_NUM_OPTIONS; i++)
    {
        length = strlen (new_server->name) + 1 +
            strlen (jabber_server_option_string[i]) + 1;
        option_name = malloc (length);
        if (option_name)
        {
            snprintf (option_name, length, "%s.%s",
                      new_server->name,
                      jabber_server_option_string[i]);
            new_server->options[i] =
                jabber_config_server_new_option (jabber_config_file,
                                                 jabber_config_section_server,
                                                 i,
                                                 option_name,
                                                 NULL,
                                                 NULL,
                                                 1,
                                                 &jabber_config_server_change_cb,
                                                 jabber_server_option_string[i]);
            jabber_config_server_change_cb (jabber_server_option_string[i],
                                            new_server->options[i]);
            free (option_name);
        }
    }
    
    return new_server;
}

/*
 * jabber_server_close_connection: close server connection
 */

void
jabber_server_close_connection (struct t_jabber_server *server)
{
    if (server->hook_fd)
    {
        weechat_unhook (server->hook_fd);
        server->hook_fd = NULL;
    }
    
    if (server->hook_connect)
    {
        weechat_unhook (server->hook_connect);
        server->hook_connect = NULL;
    }
    else
    {
#ifdef HAVE_GNUTLS
        /* close TLS connection */
        //if ((server->sock != -1) && (server->tls_connected))
        //{
        //    if (server->tls_connected)
        //        gnutls_bye (server->gnutls_sess, GNUTLS_SHUT_WR);
        //    if (server->tls_connected)
        //        gnutls_deinit (server->gnutls_sess);
        //}
#endif
    }
    if (server->iks_parser)
    {
        iks_parser_delete (server->iks_parser);
        server->iks_parser = NULL;
    }
    server->sock = -1;
    if (server->iks_id_string)
    {
        free (server->iks_id_string);
        server->iks_id_string = NULL;
    }
    server->iks_id = NULL;
    if (server->iks_server_name)
    {
        free (server->iks_server_name);
        server->iks_server_name = NULL;
    }
    if (server->iks_password)
    {
        free (server->iks_password);
        server->iks_password = NULL;
    }
    if (server->iks_filter)
    {
        iks_filter_delete (server->iks_filter);
        server->iks_filter = NULL;
    }
    server->iks_roster = NULL;
    server->iks_features = 0;
    server->iks_authorized = 0;
    
    /* remove buddies */
    jabber_buddy_free_all (server, NULL);
    
    /* server is now disconnected */
    server->is_connected = 0;
    server->tls_connected = 0;
    if (server->current_ip)
    {
        free (server->current_ip);
        server->current_ip = NULL;
    }
}

/*
 * jabber_server_reconnect_schedule: schedule reconnect for a server
 */

void
jabber_server_reconnect_schedule (struct t_jabber_server *server)
{
    int delay;
    
    if (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTORECONNECT))
    {
        server->reconnect_start = time (NULL);
        delay = JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_AUTORECONNECT_DELAY);
        weechat_printf (server->buffer,
                        _("%s%s: reconnecting to server in %d %s"),
                        jabber_buffer_get_server_prefix (server, NULL),
                        JABBER_PLUGIN_NAME,
                        delay,
                        NG_("second", "seconds", delay));
    }
    else
        server->reconnect_start = 0;
}

/*
 * jabber_server_login: login to Jabber server
 */

void
jabber_server_login (struct t_jabber_server *server)
{
    server->is_connected = 1;
    
    iks_send_header (server->iks_parser, server->iks_server_name);
}

/*
 * jabber_server_connect_cb: read connection status
 */

int
jabber_server_connect_cb (void *arg_server, int status, const char *ip_address)
{
    struct t_jabber_server *server;
    const char *proxy;
    
    server = (struct t_jabber_server *)arg_server;
    
    proxy = JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_PROXY);
    
    server->hook_connect = NULL;
    
    switch (status)
    {
        case WEECHAT_HOOK_CONNECT_OK:
            /* login to server */
            if (server->current_ip)
                free (server->current_ip);
            server->current_ip = (ip_address) ? strdup (ip_address) : NULL;
            weechat_printf (server->buffer,
                            _("%s%s: connected to %s (%s)"),
                            jabber_buffer_get_server_prefix (server, NULL),
                            JABBER_PLUGIN_NAME,
                            server->address,
                            (server->current_ip) ? server->current_ip : "?");
            server->hook_fd = weechat_hook_fd (server->sock,
                                               1, 0, 0,
                                               &jabber_server_recv_cb,
                                               server);
            jabber_server_login (server);
            break;
        case WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND:
            weechat_printf (server->buffer,
                            (proxy && proxy[0]) ?
                            _("%s%s: proxy address \"%s\" not found") :
                            _("%s%s: address \"%s\" not found"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME,
                            server->address);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND:
            weechat_printf (server->buffer,
                            (proxy && proxy[0]) ?
                            _("%s%s: proxy IP address not found") :
                            _("%s%s: IP address not found"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED:
            weechat_printf (server->buffer,
                            (proxy && proxy[0]) ?
                            _("%s%s: proxy connection refused") :
                            _("%s%s: connection refused"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_PROXY_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: proxy fails to establish "
                              "connection to server "
                              "(check username/password if used "
                              "and if server address/port is allowed by "
                              "proxy)"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: unable to set local hostname/IP"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: GnuTLS init error"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: GnuTLS handshake failed"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
        case WEECHAT_HOOK_CONNECT_MEMORY_ERROR:
            weechat_printf (server->buffer,
                            _("%s%s: not enough memory"),
                            jabber_buffer_get_server_prefix (server, "error"),
                            JABBER_PLUGIN_NAME);
            jabber_server_close_connection (server);
            jabber_server_reconnect_schedule (server);
            break;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_server_set_buffer_title: set title for a server buffer
 */

void
jabber_server_set_buffer_title (struct t_jabber_server *server)
{
    char *title;
    int length;
    
    if (server && server->buffer)
    {
        if (server->is_connected)
        {
            length = 16 + strlen (server->address) + 16 +
                ((server->current_ip) ? strlen (server->current_ip) : 16) + 1;
            title = malloc (length);
            if (title)
            {
                snprintf (title, length, "Jabber: %s/%d (%s)",
                          server->address, server->port,
                          (server->current_ip) ? server->current_ip : "");
                weechat_buffer_set (server->buffer, "title", title);
                free (title);
            }
        }
        else
        {
            weechat_buffer_set (server->buffer, "title", "");
        }
    }
}

/*
 * jabber_server_create_buffer: create a buffer for a Jabber server
 */

struct t_gui_buffer *
jabber_server_create_buffer (struct t_jabber_server *server, int all_servers)
{
    char buffer_name[256], charset_modifier[256];
    const char *local_name;
    
    if (all_servers)
    {
        snprintf (buffer_name, sizeof (buffer_name),
                  JABBER_BUFFER_ALL_SERVERS_NAME);
    }
    else
    {
        snprintf (buffer_name, sizeof (buffer_name),
                  "server.%s", server->name);
    }
    server->buffer = weechat_buffer_new (buffer_name,
                                         NULL, NULL,
                                         &jabber_buffer_close_cb, NULL);
    if (!server->buffer)
        return NULL;
    
    weechat_buffer_set (server->buffer, "short_name",
                        (weechat_config_boolean (jabber_config_look_one_server_buffer)) ?
                        JABBER_BUFFER_ALL_SERVERS_NAME : server->name);
    weechat_buffer_set (server->buffer, "localvar_set_server",
                        (weechat_config_boolean (jabber_config_look_one_server_buffer)) ?
                        JABBER_BUFFER_ALL_SERVERS_NAME : server->name);
    weechat_buffer_set (server->buffer, "localvar_set_muc",
                        (weechat_config_boolean (jabber_config_look_one_server_buffer)) ?
                        JABBER_BUFFER_ALL_SERVERS_NAME : server->name);
    snprintf (charset_modifier, sizeof (charset_modifier),
              "jabber.%s", server->name);
    weechat_buffer_set (server->buffer, "localvar_set_charset_modifier",
                        charset_modifier);
    
    weechat_buffer_set (server->buffer, "nicklist", "1");
    weechat_buffer_set (server->buffer, "nicklist_display_groups", "0");
    
    weechat_hook_signal_send ("logger_backlog",
                              WEECHAT_HOOK_SIGNAL_POINTER, server->buffer);
    
    /* set highlights settings on server buffer */
    local_name = jabber_server_get_local_name (server);
    if (local_name && local_name[0])
        weechat_buffer_set (server->buffer, "highlight_words", local_name);
    if (weechat_config_string (jabber_config_look_highlight_tags)
        && weechat_config_string (jabber_config_look_highlight_tags)[0])
    {
        weechat_buffer_set (server->buffer, "highlight_tags",
                            weechat_config_string (jabber_config_look_highlight_tags));
    }
    
    jabber_server_set_buffer_title (server);
    
    return server->buffer;
}

/*
 * jabber_server_set_current_server: set new current server (when all servers
 *                                   are in one buffer)
 */

void
jabber_server_set_current_server (struct t_jabber_server *server)
{
    char charset_modifier[256];
    
    jabber_current_server = server;
    
    jabber_server_set_buffer_title (jabber_current_server);
    snprintf (charset_modifier, sizeof (charset_modifier),
              "jabber.%s", jabber_current_server->name);
    weechat_buffer_set (jabber_current_server->buffer,
                        "localvar_set_charset_modifier",
                        charset_modifier);
    weechat_bar_item_update ("buffer_name");
    weechat_bar_item_update ("input_prompt");
}

/*
 * jabber_server_iks_transport_connect_async: async connection to server
 *                                            (for iksemel lib)
 */

int
jabber_server_iks_transport_connect_async (iksparser *parser, void **socketptr,
                                           const char *server,
                                           const char *server_name,
                                           int port, void *notify_data,
                                           iksAsyncNotify *notify_func)
{
    struct t_jabber_server *ptr_server;
    int set, length;
    char *option_name;
    struct t_config_option *proxy_type, *proxy_ipv6, *proxy_address, *proxy_port;
    const char *proxy, *str_proxy_type, *str_proxy_address;
    
    /* make C compiler happy */
    (void) parser;
    (void) server_name;
    (void) notify_func;
    
    ptr_server = (struct t_jabber_server *)notify_data;
    
    proxy_type = NULL;
    proxy_ipv6 = NULL;
    proxy_address = NULL;
    proxy_port = NULL;
    str_proxy_type = NULL;
    str_proxy_address = NULL;
    
    proxy = JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_PROXY);
    if (proxy && proxy[0])
    {
        length = 32 + strlen (proxy) + 1;
        option_name = malloc (length);
        if (!option_name)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: not enough memory"),
                            jabber_buffer_get_server_prefix (ptr_server, "error"),
                            JABBER_PLUGIN_NAME);
            return 0;
        }
        snprintf (option_name, length, "weechat.proxy.%s.type", proxy);
        proxy_type = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.ipv6", proxy);
        proxy_ipv6 = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.address", proxy);
        proxy_address = weechat_config_get (option_name);
        snprintf (option_name, length, "weechat.proxy.%s.port", proxy);
        proxy_port = weechat_config_get (option_name);
        free (option_name);
        if (!proxy_type || !proxy_address)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: proxy \"%s\" not found for server "
                              "\"%s\", cannot connect"),
                            jabber_buffer_get_server_prefix (ptr_server, "error"),
                            JABBER_PLUGIN_NAME, proxy, ptr_server->name);
            return 0;
        }
        str_proxy_type = weechat_config_string (proxy_type);
        str_proxy_address = weechat_config_string (proxy_address);
        if (!str_proxy_type[0] || !proxy_ipv6 || !str_proxy_address[0]
            || !proxy_port)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: missing proxy settings, check options "
                              "for proxy \"%s\""),
                            jabber_buffer_get_server_prefix (ptr_server, "error"),
                            JABBER_PLUGIN_NAME, proxy);
            return 0;
        }
    }
    
    if (proxy_type)
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: connecting to server %s/%d%s%s%s via %s "
                          "proxy %s/%d%s..."),
                        jabber_buffer_get_server_prefix (ptr_server, NULL),
                        JABBER_PLUGIN_NAME,
                        server,
                        port,
                        (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6)) ?
                        " (IPv6)" : "",
                        (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_TLS)) ?
                        " (TLS)" : "",
                        (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_SASL)) ?
                        " (SASL)" : "",
                        str_proxy_type,
                        str_proxy_address,
                        weechat_config_integer (proxy_port),
                        (weechat_config_boolean (proxy_ipv6)) ? " (IPv6)" : "");
        weechat_log_printf (_("Connecting to server %s/%d%s%s%s via %s proxy "
                              "%s/%d%s..."),
                            server,
                            port,
                            (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6)) ?
                            " (IPv6)" : "",
                            (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_TLS)) ?
                            " (TLS)" : "",
                            (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_SASL)) ?
                            " (SASL)" : "",
                            str_proxy_type,
                            str_proxy_address,
                            weechat_config_integer (proxy_port),
                            (weechat_config_boolean (proxy_ipv6)) ? " (IPv6)" : "");
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: connecting to server %s/%d%s%s%s..."),
                        jabber_buffer_get_server_prefix (ptr_server, NULL),
                        JABBER_PLUGIN_NAME,
                        server,
                        port,
                        (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6)) ?
                        " (IPv6)" : "",
                        (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_TLS)) ?
                        " (TLS)" : "",
                        (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_SASL)) ?
                        " (SASL)" : "");
        weechat_log_printf (_("%s%s: connecting to server %s/%d%s%s%s..."),
                            "",
                            JABBER_PLUGIN_NAME,
                            server,
                            port,
                            (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6)) ?
                            " (IPv6)" : "",
                            (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_TLS)) ?
                            " (TLS)" : "",
                            (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_SASL)) ?
                            " (SASL)" : "");
    }
    
    /* create socket and set options */
    if (proxy_type)
    {
        ptr_server->sock = socket ((weechat_config_integer (proxy_ipv6)) ?
                                   AF_INET6 : AF_INET,
                                   SOCK_STREAM, 0);
    }
    else
    {
        ptr_server->sock = socket ((JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6)) ?
                                   AF_INET6 : AF_INET,
                                   SOCK_STREAM, 0);
    }
    if (ptr_server->sock == -1)
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: cannot create socket"),
                        jabber_buffer_get_server_prefix (ptr_server, "error"),
                        JABBER_PLUGIN_NAME);
        return 0;
    }
    
    /* set SO_REUSEADDR option for socket */
    set = 1;
    if (setsockopt (ptr_server->sock, SOL_SOCKET, SO_REUSEADDR,
        (void *) &set, sizeof (set)) == -1)
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: cannot set socket option "
                          "\"SO_REUSEADDR\""),
                        jabber_buffer_get_server_prefix (ptr_server, "error"),
                        JABBER_PLUGIN_NAME);
    }
    
    /* set SO_KEEPALIVE option for socket */
    set = 1;
    if (setsockopt (ptr_server->sock, SOL_SOCKET, SO_KEEPALIVE,
        (void *) &set, sizeof (set)) == -1)
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: cannot set socket option "
                          "\"SO_KEEPALIVE\""),
                        jabber_buffer_get_server_prefix (ptr_server, "error"),
                        JABBER_PLUGIN_NAME);
    }
    
    *socketptr = (void *) ptr_server->sock;
    
    /* init TLS if asked */
    ptr_server->tls_connected = 0;
#ifdef HAVE_GNUTLS
    if (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_TLS))
        ptr_server->tls_connected = 1;
#endif
    
    ptr_server->hook_connect = weechat_hook_connect (proxy,
                                                     server,
                                                     port,
                                                     ptr_server->sock,
                                                     JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6),
#ifdef HAVE_GNUTLS
                                                     //(ptr_server->tls_connected) ? &ptr_server->gnutls_sess : NULL,
                                                     NULL,
#else
                                                     NULL,
#endif
                                                     JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_LOCAL_HOSTNAME),
                                                     &jabber_server_connect_cb,
                                                     ptr_server);
    
    /* send signal "jabber_server_connecting" with server name */
    weechat_hook_signal_send ("jabber_server_connecting",
                              WEECHAT_HOOK_SIGNAL_STRING, ptr_server->name);
    
    return IKS_OK;
}

/*
 * jabber_server_iks_transport_send: send data to server (for iksemel lib)
 */

int
jabber_server_iks_transport_send (void *socket, const char *data, size_t len)
{
    if (send ((int) socket, data, len, 0) == -1)
        return IKS_NET_RWERR;
    
    return IKS_OK;
}

/*
 * jabber_server_iks_transport_recv: recv data from server (for iksemel lib)
 */

int
jabber_server_iks_transport_recv (void *socket, char *buffer, size_t buf_len,
                                  int timeout)
{
    int sock;
    fd_set fds;
    struct timeval tv, *ptr_tv;
    int len;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    sock = (int) socket;
    FD_ZERO (&fds);
    FD_SET (sock, &fds);
    tv.tv_sec = timeout;
    ptr_tv = (timeout != -1) ? &tv : NULL;
    
    /* FIXME: force timeout to 1 second, otherwise WeeChat may freeze when
       iksemel calls this function with -1 (for TLS connection) */
    tv.tv_sec = 1;
    ptr_tv = &tv;
    
    if (select (sock + 1, &fds, NULL, NULL, ptr_tv) > 0)
    {
        len = recv (sock, buffer, buf_len, 0);
        if (len > 0)
            return len;
        else if (len <= 0)
            return -1;
    }
    
    return 0;
}

/*
 * jabber_server_iks_transport_close: close connection with server
 *                                    (for iksemel lib)
 */

void
jabber_server_iks_transport_close (void *socket)
{
    int sock = (int)socket;
    
#ifdef _WIN32
    closesocket (sock);
#else
    close (sock);
#endif
}

/*
 * jabber_server_connect: connect to a Jabber server
 *                        Return: 1 if ok
 *                                0 if error
 */

int
jabber_server_connect (struct t_jabber_server *server)
{
    int length;
    char charset_modifier[256];
    const char *username, *resource, *password;
    
    username = JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_USERNAME);
    resource = JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_RESOURCE);
    if (!username || !username[0] || !server->address || !server->address[0])
    {
        weechat_printf (server->buffer,
                        _("%s%s: username or server not defined for server "
                          "\"%s\", cannot connect"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME, server->name);
        return 0;
    }
    
    if (!server->buffer)
    {
        if (weechat_config_boolean (jabber_config_look_one_server_buffer)
            && jabber_buffer_servers)
        {
            server->buffer = jabber_buffer_servers;
            jabber_server_set_buffer_title (server);
        }
        else
        {
            if (!jabber_server_create_buffer (server,
                                              weechat_config_boolean (jabber_config_look_one_server_buffer)))
                return 0;
        }
        
        if (weechat_config_boolean (jabber_config_look_one_server_buffer))
        {
            jabber_current_server = server;
            if (!jabber_buffer_servers)
                jabber_buffer_servers = server->buffer;
            
            snprintf (charset_modifier, sizeof (charset_modifier),
                      "jabber.%s", jabber_current_server->name);
            weechat_buffer_set (jabber_buffer_servers,
                                "localvar_set_charset_modifier",
                                charset_modifier);
        }
        
        weechat_buffer_set (server->buffer, "display", "auto");
        
        weechat_bar_item_update ("buffer_name");
        
        weechat_buffer_set (server->buffer, "key_bind_meta-s",
                            "/command jabber /jabber switch");
    }

    if (!server->address)
    {
        weechat_printf (server->buffer,
                        _("%s%s: hostname/IP not defined for server \"%s\", "
                          "cannot connect"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME, server->name);
        return 0;
    }
    
#ifndef HAVE_GNUTLS
    //if (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_TLS))
    //{
    //    weechat_printf (server->buffer,
    //                    _("%s%s: cannot connect with TLS because WeeChat "
    //                      "was not built with GnuTLS support"),
    //                    jabber_buffer_get_server_prefix (server, "error"),
    //                    JABBER_PLUGIN_NAME);
    //    return 0;
    //}
#endif
    
    if (JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_TLS)
        && !iks_has_tls ())
    {
        weechat_printf (server->buffer,
                        _("%s%s: cannot connect with TLS because iksemel "
                          "library was not built with GnuTLS support"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME);
        return 0;
    }
    
    /* close connection if opened */
    jabber_server_close_connection (server);
    
    /* build jabber ID */
    length = strlen (username) + 1 + strlen (server->address) + 1 +
        strlen ((resource && resource[0]) ? resource : JABBER_SERVER_DEFAULT_RESOURCE) + 1;
    server->iks_id_string = malloc (length);
    if (!server->iks_id_string)
    {
        weechat_printf (server->buffer,
                        _("%s%s: not enough memory"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME);
        return 0;
    }
    snprintf (server->iks_id_string, length, "%s@%s/%s",
              username, server->address,
              (resource && resource[0]) ?
              resource : JABBER_SERVER_DEFAULT_RESOURCE);
    server->iks_parser = iks_stream_new (IKS_NS_CLIENT, server,
                                         &jabber_xmpp_iks_stream_hook);
    if (!server->iks_parser)
    {
        weechat_printf (server->buffer,
                        _("%s%s: failed to create stream parser"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME);
        return 0;
    }
    server->iks_id = iks_id_new (iks_parser_stack (server->iks_parser),
                                 server->iks_id_string);
    if (!server->iks_id)
    {
        iks_parser_delete (server->iks_parser);
        server->iks_parser = NULL;
        free (server->iks_id_string);
        server->iks_id_string = NULL;
        weechat_printf (server->buffer,
                        _("%s%s: failed to create id"),
                        jabber_buffer_get_server_prefix (server, "error"),
                        JABBER_PLUGIN_NAME);
        return 0;
    }
    server->iks_server_name = strdup (server->address);
    password = JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_PASSWORD);
    server->iks_password = strdup ((password) ? password : "");
    iks_set_log_hook (server->iks_parser, &jabber_xmpp_iks_log);
    server->iks_filter = iks_filter_new ();
    iks_filter_add_rule (server->iks_filter,
                         &jabber_xmpp_iks_result, server,
                         IKS_RULE_TYPE, IKS_PAK_IQ,
                         IKS_RULE_SUBTYPE, IKS_TYPE_RESULT,
                         IKS_RULE_ID, "auth",
                         IKS_RULE_DONE);
    iks_filter_add_rule (server->iks_filter,
                         &jabber_xmpp_iks_error, server,
                         IKS_RULE_TYPE, IKS_PAK_IQ,
                         IKS_RULE_SUBTYPE, IKS_TYPE_ERROR,
                         IKS_RULE_ID, "auth",
                         IKS_RULE_DONE);
    iks_filter_add_rule (server->iks_filter,
                         &jabber_xmpp_iks_roster, server,
                         IKS_RULE_TYPE, IKS_PAK_IQ,
                         IKS_RULE_SUBTYPE, IKS_TYPE_RESULT,
                         IKS_RULE_ID, "roster",
                         IKS_RULE_DONE);
    iks_connect_async_with (server->iks_parser, server->address, server->port,
                            server->iks_server_name, &jabber_iks_transport,
                            server, NULL);
    
    return 1;
}

/*
 * jabber_server_reconnect: reconnect to a server (after disconnection)
 */

void
jabber_server_reconnect (struct t_jabber_server *server)
{
    weechat_printf (server->buffer,
                    _("%s%s: reconnecting to server..."),
                    jabber_buffer_get_server_prefix (server, NULL),
                    JABBER_PLUGIN_NAME);
    server->reconnect_start = 0;
    
    if (jabber_server_connect (server))
        server->reconnect_join = 1;
    else
        jabber_server_reconnect_schedule (server);
}

/*
 * jabber_server_auto_connect: auto-connect to servers (called at startup)
 */

void
jabber_server_auto_connect ()
{
    struct t_jabber_server *ptr_server;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        if (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_AUTOCONNECT))
        {
            if (!jabber_server_connect (ptr_server))
                jabber_server_reconnect_schedule (ptr_server);
        }
    }
}

/*
 * jabber_server_disconnect: disconnect from a Jabber server
 */

void
jabber_server_disconnect (struct t_jabber_server *server, int reconnect)
{
    struct t_jabber_muc *ptr_muc;
    
    if (server->is_connected)
    {
        /* remove all buddies and write disconnection message on each
           MUC/private buffer */
        for (ptr_muc = server->mucs; ptr_muc; ptr_muc = ptr_muc->next_muc)
        {
            jabber_buddy_free_all (NULL, ptr_muc);
            weechat_printf (ptr_muc->buffer,
                            _("%s%s: disconnected from server"),
                            "",
                            JABBER_PLUGIN_NAME);
        }
    }
    
    jabber_server_close_connection (server);
    
    if (server->buffer)
    {
        weechat_printf (server->buffer,
                        _("%s%s: disconnected from server"),
                        jabber_buffer_get_server_prefix (server, NULL),
                        JABBER_PLUGIN_NAME);
    }
    
    server->is_away = 0;
    server->away_time = 0;
    server->lag = 0;
    server->lag_check_time.tv_sec = 0;
    server->lag_check_time.tv_usec = 0;
    server->lag_next_check = time (NULL) +
        weechat_config_integer (jabber_config_network_lag_check);
    
    if (reconnect
        && JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTORECONNECT))
        jabber_server_reconnect_schedule (server);
    else
        server->reconnect_start = 0;
    
    jabber_server_set_buffer_title (server);
    
    /* send signal "jabber_server_disconnected" with server name */
    weechat_hook_signal_send ("jabber_server_disconnected",
                              WEECHAT_HOOK_SIGNAL_STRING, server->name);
}

/*
 * jabber_server_disconnect_all: disconnect from all jabber servers
 */

void
jabber_server_disconnect_all ()
{
    struct t_jabber_server *ptr_server;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        jabber_server_disconnect (ptr_server, 0);
    }
}

/*
 * jabber_server_free_data: free server data
 */

void
jabber_server_free_data (struct t_jabber_server *server)
{
    int i;
    
    if (!server)
        return;
    
    /* free data */
    for (i = 0; i < JABBER_SERVER_NUM_OPTIONS; i++)
    {
        if (server->options[i])
            weechat_config_option_free (server->options[i]);
    }
    if (server->name)
        free (server->name);
    if (server->address)
        free (server->address);
    if (server->current_ip)
        free (server->current_ip);
    if (server->iks_parser)
        iks_parser_delete (server->iks_parser);
    if (server->iks_id_string)
        free (server->iks_id_string);
    if (server->iks_server_name)
        free (server->iks_server_name);
    if (server->iks_password)
        free (server->iks_password);
    if (server->iks_filter)
        iks_filter_delete (server->iks_filter);
    if (server->away_message)
        free (server->away_message);
    if (server->mucs)
        jabber_muc_free_all (server);
    if (server->buddies)
        jabber_buddy_free_all (server, NULL);
    if (server->buffer_as_string)
        free (server->buffer_as_string);
}

/*
 * jabber_server_free: free a server and remove it from servers queue
 */

void
jabber_server_free (struct t_jabber_server *server)
{
    struct t_jabber_server *new_jabber_servers;
    
    if (!server)
        return;
    
    /* close all MUCs/privates */
    jabber_muc_free_all (server);
    
    /* remove server from queue */
    if (last_jabber_server == server)
        last_jabber_server = server->prev_server;
    if (server->prev_server)
    {
        (server->prev_server)->next_server = server->next_server;
        new_jabber_servers = jabber_servers;
    }
    else
        new_jabber_servers = server->next_server;
    
    if (server->next_server)
        (server->next_server)->prev_server = server->prev_server;
    
    jabber_server_free_data (server);
    free (server);
    jabber_servers = new_jabber_servers;
}

/*
 * jabber_server_free_all: free all allocated servers
 */

void
jabber_server_free_all ()
{
    /* for each server in memory, remove it */
    while (jabber_servers)
    {
        jabber_server_free (jabber_servers);
    }
}

/*
 * jabber_server_copy: copy a server
 *                     return: pointer to new server, NULL if error
 */

struct t_jabber_server *
jabber_server_copy (struct t_jabber_server *server, const char *new_name)
{
    struct t_jabber_server *new_server;
    struct t_infolist *infolist;
    char *mask, *pos;
    const char *option_name;
    int length, index_option;
    
    /* check if another server exists with this name */
    if (jabber_server_search (new_name))
        return NULL;
    
    new_server = jabber_server_alloc (new_name);
    if (new_server)
    {
        /* duplicate options */
        length = 32 + strlen (server->name) + 1;
        mask = malloc (length);
        if (!mask)
            return 0;
        snprintf (mask, length, "jabber.server.%s.*", server->name);
        infolist = weechat_infolist_get ("option", NULL, mask);
        free (mask);
        while (weechat_infolist_next (infolist))
        {
            if (!weechat_infolist_integer (infolist, "value_is_null"))
            {
                option_name = weechat_infolist_string (infolist, "option_name");
                pos = strrchr (option_name, '.');
                if (pos)
                {
                    index_option = jabber_server_search_option (pos + 1);
                    if (index_option >= 0)
                    {
                        weechat_config_option_set (new_server->options[index_option],
                                                   weechat_infolist_string (infolist, "value"),
                                                   1);
                    }
                }
            }
        }
    }
    
    return new_server;
}

/*
 * jabber_server_rename: rename server (internal name)
 *                       return: 1 if ok, 0 if error
 */

int
jabber_server_rename (struct t_jabber_server *server,
                      const char *new_server_name)
{
    int length;
    char *mask, *pos_option, *new_option_name, *buffer_name;
    const char *option_name;
    struct t_infolist *infolist;
    struct t_config_option *ptr_option;
    //struct t_jabber_channel *ptr_channel;
    
    /* check if another server exists with this name */
    if (jabber_server_search (new_server_name))
        return 0;
    
    /* rename options */
    length = 32 + strlen (server->name) + 1;
    mask = malloc (length);
    if (!mask)
        return 0;
    snprintf (mask, length, "jabber.server.%s.*", server->name);
    infolist = weechat_infolist_get ("option", NULL, mask);
    free (mask);
    while (weechat_infolist_next (infolist))
    {
        weechat_config_search_with_string (weechat_infolist_string (infolist,
                                                                    "full_name"),
                                           NULL, NULL, &ptr_option,
                                           NULL);
        if (ptr_option)
        {
            option_name = weechat_infolist_string (infolist, "option_name");
            if (option_name)
            {
                pos_option = strrchr (option_name, '.');
                if (pos_option)
                {
                    pos_option++;
                    length = strlen (new_server_name) + 1 + strlen (pos_option) + 1;
                    new_option_name = malloc (length);
                    if (new_option_name)
                    {
                        snprintf (new_option_name, length,
                                  "%s.%s", new_server_name, pos_option);
                        weechat_config_option_rename (ptr_option, new_option_name);
                        free (new_option_name);
                    }
                }
            }
        }
    }
    weechat_infolist_free (infolist);
    
    /* rename server */
    if (server->name)
        free (server->name);
    server->name = strdup (new_server_name);
    
    /* change name for buffers with this server */
    /*
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->buffer)
        {
            buffer_name = jabber_buffer_build_name (server->name, ptr_channel->name);
            weechat_buffer_set (ptr_channel->buffer, "name", buffer_name);
        }
    }
    */
    if (server->buffer)
    {
        buffer_name = jabber_buffer_build_name (server->name, NULL);
        weechat_buffer_set (server->buffer, "name", buffer_name);
    }
    
    return 1;
}

/*
 * jabber_server_send_signal: send a signal for a Jabber message (received or sent)
 */

void
jabber_server_send_signal (struct t_jabber_server *server, const char *signal,
                           const char *command, const char *full_message)
{
    int length;
    char *str_signal;
    
    length = strlen (server->name) + 1 + strlen (signal) + 1 + strlen (command) + 1;
    str_signal = malloc (length);
    if (str_signal)
    {
        snprintf (str_signal, length,
                  "%s,%s_%s", server->name, signal, command);
        weechat_hook_signal_send (str_signal, WEECHAT_HOOK_SIGNAL_STRING,
                                  (void *)full_message);
        free (str_signal);
    }
}

/*
 * jabber_server_recv_cb: receive data from a jabber server
 */

int
jabber_server_recv_cb (void *arg_server)
{
    struct t_jabber_server *server;
    int rc;
    
    server = (struct t_jabber_server *)arg_server;
    
    if (!server)
        return WEECHAT_RC_ERROR;
    
    rc = iks_recv (server->iks_parser, 1);
    
    if (rc == IKS_NET_TLSFAIL)
    {
        weechat_printf (server->buffer,
                        _("%s%s: TLS handshake failed"),
                        jabber_buffer_get_server_prefix (server,
                                                         "error"),
                        JABBER_PLUGIN_NAME);
        jabber_server_disconnect (server, 0);
        return WEECHAT_RC_ERROR;
    }
    
    if ((rc != IKS_OK) && (rc != IKS_HOOK))
    {
        weechat_printf (server->buffer,
                        _("%s%s: I/O error (%d)"),
                        jabber_buffer_get_server_prefix (server,
                                                         "error"),
                        JABBER_PLUGIN_NAME,
                        rc);
        jabber_server_disconnect (server, 0);
        return WEECHAT_RC_ERROR;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_server_timer_cb: timer called each second to perform some operations
 *                         on servers
 */

int
jabber_server_timer_cb (void *data)
{
    struct t_jabber_server *ptr_server;
    time_t new_time;
    //static struct timeval tv;
    //int diff;
    
    /* make C compiler happy */
    (void) data;
    
    new_time = time (NULL);
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        /* check if reconnection is pending */
        if ((!ptr_server->is_connected)
            && (ptr_server->reconnect_start > 0)
            && (new_time >= (ptr_server->reconnect_start +
                             JABBER_SERVER_OPTION_INTEGER(ptr_server, JABBER_SERVER_OPTION_AUTORECONNECT_DELAY))))
            jabber_server_reconnect (ptr_server);
        else
        {
            if (ptr_server->is_connected)
            {
                /* check for lag */
                //if ((ptr_server->lag_check_time.tv_sec == 0)
                //    && (new_time >= ptr_server->lag_next_check))
                //{
                //    jabber_server_sendf (ptr_server, "PING %s",
                //                         ptr_server->addresses_array[ptr_server->index_current_address]);
                //    gettimeofday (&(ptr_server->lag_check_time), NULL);
                //}
                
                /* check if it's time to autojoin channels (after command delay) */
                //if ((ptr_server->command_time != 0)
                //    && (new_time >= ptr_server->command_time +
                //        JABBER_SERVER_OPTION_INTEGER(ptr_server, JABBER_SERVER_OPTION_COMMAND_DELAY)))
                //{
                //    jabber_server_autojoin_channels (ptr_server);
                //    ptr_server->command_time = 0;
                //}
                
                /* lag timeout => disconnect */
                //if ((ptr_server->lag_check_time.tv_sec != 0)
                //    && (weechat_config_integer (jabber_config_network_lag_disconnect) > 0))
                //{
                //    gettimeofday (&tv, NULL);
                //    diff = (int) weechat_timeval_diff (&(ptr_server->lag_check_time),
                //                                       &tv);
                //    if (diff / 1000 > weechat_config_integer (jabber_config_network_lag_disconnect) * 60)
                //    {
                //        weechat_printf (ptr_server->buffer,
                //                        _("%s%s: lag is high, "
                //                          "disconnecting from "
                //                          "server..."),
                //                       jabber_buffer_get_server_prefix (ptr_server,
                //                                                      NULL),
                //                       JABBER_PLUGIN_NAME);
                //        jabber_server_disconnect (ptr_server, 1);
                //    }
                //}
            }
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * jabber_server_add_to_infolist: add a server in an infolist
 *                                return 1 if ok, 0 if error
 */

int
jabber_server_add_to_infolist (struct t_infolist *infolist,
                               struct t_jabber_server *server)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !server)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_string (ptr_item, "name", server->name))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "buffer", server->buffer))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_name",
                                          (server->buffer) ?
                                          weechat_buffer_get_string (server->buffer, "name") : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "buffer_short_name",
                                          (server->buffer) ?
                                          weechat_buffer_get_string (server->buffer, "short_name") : ""))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "selected",
                                           (weechat_config_boolean (jabber_config_look_one_server_buffer)
                                            && (jabber_current_server != server)) ?
                                           0 : 1))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "username",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_USERNAME)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "server",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_SERVER)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "proxy",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_PROXY)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "ipv6",
                                           JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_IPV6)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "tls",
                                           JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_TLS)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sasl",
                                           JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_SASL)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "resource",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_RESOURCE)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "password",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_PASSWORD)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_alias",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_LOCAL_ALIAS)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoconnect",
                                           JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTOCONNECT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoreconnect",
                                           JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTORECONNECT)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autoreconnect_delay",
                                           JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_AUTORECONNECT_DELAY)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "local_hostname",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_LOCAL_HOSTNAME)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "command",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_COMMAND)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "command_delay",
                                           JABBER_SERVER_OPTION_INTEGER(server, JABBER_SERVER_OPTION_COMMAND_DELAY)))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "autojoin",
                                          JABBER_SERVER_OPTION_STRING(server, JABBER_SERVER_OPTION_AUTOJOIN)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "autorejoin",
                                           JABBER_SERVER_OPTION_BOOLEAN(server, JABBER_SERVER_OPTION_AUTOREJOIN)))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "temp_server", server->temp_server))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "address", server->address))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "port", server->port))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "current_ip", server->current_ip))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "sock", server->sock))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "iks_id_string", server->iks_id_string))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "iks_server_name", server->iks_server_name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "iks_password", server->iks_password))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "iks_features", server->iks_features))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "iks_authorized", server->iks_authorized))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_connected", server->is_connected))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "tls_connected", server->tls_connected))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "reconnect_start", server->reconnect_start))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "command_time", server->command_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "reconnect_join", server->reconnect_join))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "disable_autojoin", server->disable_autojoin))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "is_away", server->is_away))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "away_message", server->away_message))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "away_time", server->away_time))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "lag", server->lag))
        return 0;
    if (!weechat_infolist_new_var_buffer (ptr_item, "lag_check_time", &(server->lag_check_time), sizeof (struct timeval)))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "lag_next_check", server->lag_next_check))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "buddies_count", server->buddies_count))
        return 0;
    
    return 1;
}

/*
 * jabber_server_print_log: print server infos in log (usually for crash dump)
 */

void
jabber_server_print_log ()
{
    struct t_jabber_server *ptr_server;
    struct t_jabber_buddy *ptr_buddy;
    struct t_jabber_muc *ptr_muc;
    
    for (ptr_server = jabber_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[server %s (addr:0x%lx)]", ptr_server->name, ptr_server);

        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_USERNAME]))
            weechat_log_printf ("  username . . . . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_USERNAME));
        else
            weechat_log_printf ("  username . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_USERNAME]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_SERVER]))
            weechat_log_printf ("  server . . . . . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_SERVER));
        else
            weechat_log_printf ("  server . . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_SERVER]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_PROXY]))
            weechat_log_printf ("  proxy. . . . . . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_PROXY));
        else
            weechat_log_printf ("  proxy. . . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_PROXY]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_IPV6]))
            weechat_log_printf ("  ipv6 . . . . . . . . : null (%s)",
                                (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_IPV6)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  ipv6 . . . . . . . . : %s",
                                weechat_config_boolean (ptr_server->options[JABBER_SERVER_OPTION_IPV6]) ?
                                "on" : "off");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_TLS]))
            weechat_log_printf ("  tls. . . . . . . . . : null (%s)",
                                (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_TLS)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  tls. . . . . . . . . : %s",
                                weechat_config_boolean (ptr_server->options[JABBER_SERVER_OPTION_TLS]) ?
                                "on" : "off");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_SASL]))
            weechat_log_printf ("  sasl . . . . . . . . : null (%s)",
                                (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_SASL)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  sasl . . . . . . . . : %s",
                                weechat_config_boolean (ptr_server->options[JABBER_SERVER_OPTION_SASL]) ?
                                "on" : "off");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_RESOURCE]))
            weechat_log_printf ("  resource . . . . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_RESOURCE));
        else
            weechat_log_printf ("  resource . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_RESOURCE]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_PASSWORD]))
            weechat_log_printf ("  password . . . . . . : null");
        else
            weechat_log_printf ("  password . . . . . . : (hidden)");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_LOCAL_ALIAS]))
            weechat_log_printf ("  local_alias. . . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_LOCAL_ALIAS));
        else
            weechat_log_printf ("  local_alias. . . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_LOCAL_ALIAS]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_AUTOCONNECT]))
            weechat_log_printf ("  autoconnect. . . . . : null (%s)",
                                (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_AUTOCONNECT)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  autoconnect. . . . . : %s",
                                weechat_config_boolean (ptr_server->options[JABBER_SERVER_OPTION_AUTOCONNECT]) ?
                                "on" : "off");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_AUTORECONNECT]))
            weechat_log_printf ("  autoreconnect. . . . : null (%s)",
                                (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_AUTORECONNECT)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  autoreconnect. . . . : %s",
                                weechat_config_boolean (ptr_server->options[JABBER_SERVER_OPTION_AUTORECONNECT]) ?
                                "on" : "off");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_AUTORECONNECT_DELAY]))
            weechat_log_printf ("  autoreconnect_delay. : null (%d)",
                                JABBER_SERVER_OPTION_INTEGER(ptr_server, JABBER_SERVER_OPTION_AUTORECONNECT_DELAY));
        else
            weechat_log_printf ("  autoreconnect_delay. : %d",
                                weechat_config_integer (ptr_server->options[JABBER_SERVER_OPTION_AUTORECONNECT_DELAY]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_LOCAL_HOSTNAME]))
            weechat_log_printf ("  local_hostname . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_LOCAL_HOSTNAME));
        else
            weechat_log_printf ("  local_hostname . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_LOCAL_HOSTNAME]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_COMMAND]))
            weechat_log_printf ("  command. . . . . . . : null");
        else
            weechat_log_printf ("  command. . . . . . . : (hidden)");
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_COMMAND_DELAY]))
            weechat_log_printf ("  command_delay. . . . : null (%d)",
                                JABBER_SERVER_OPTION_INTEGER(ptr_server, JABBER_SERVER_OPTION_COMMAND_DELAY));
        else
            weechat_log_printf ("  command_delay. . . . : %d",
                                weechat_config_integer (ptr_server->options[JABBER_SERVER_OPTION_COMMAND_DELAY]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_AUTOJOIN]))
            weechat_log_printf ("  autojoin . . . . . . : null ('%s')",
                                JABBER_SERVER_OPTION_STRING(ptr_server, JABBER_SERVER_OPTION_AUTOJOIN));
        else
            weechat_log_printf ("  autojoin . . . . . . : '%s'",
                                weechat_config_string (ptr_server->options[JABBER_SERVER_OPTION_AUTOJOIN]));
        if (weechat_config_option_is_null (ptr_server->options[JABBER_SERVER_OPTION_AUTOREJOIN]))
            weechat_log_printf ("  autorejoin . . . . . : null (%s)",
                                (JABBER_SERVER_OPTION_BOOLEAN(ptr_server, JABBER_SERVER_OPTION_AUTOREJOIN)) ?
                                "on" : "off");
        else
            weechat_log_printf ("  autorejoin . . . . . : %s",
                                weechat_config_boolean (ptr_server->options[JABBER_SERVER_OPTION_AUTOREJOIN]) ?
                                "on" : "off");
        weechat_log_printf ("  temp_server. . . . . : %d",    ptr_server->temp_server);
        weechat_log_printf ("  reloading_from_config: %d",    ptr_server->reloaded_from_config);
        weechat_log_printf ("  reloaded_from_config : %d",    ptr_server->reloaded_from_config);
        weechat_log_printf ("  address. . . . . . . : '%s'",  ptr_server->address);
        weechat_log_printf ("  port . . . . . . . . : %d",    ptr_server->port);
        weechat_log_printf ("  current_ip . . . . . : '%s'",  ptr_server->current_ip);
        weechat_log_printf ("  sock . . . . . . . . : %d",    ptr_server->sock);
        weechat_log_printf ("  iks_parser . . . . . : 0x%lx", ptr_server->iks_parser);
        weechat_log_printf ("  iks_id_string. . . . : '%s'",  ptr_server->iks_id_string);
        weechat_log_printf ("  iks_id . . . . . . . : 0x%lx", ptr_server->iks_id);
        weechat_log_printf ("  iks_server_name. . . : '%s'",  ptr_server->iks_server_name);
        weechat_log_printf ("  iks_password . . . . : (hidden)");
        weechat_log_printf ("  iks_filter . . . . . : 0x%lx", ptr_server->iks_filter);
        weechat_log_printf ("  iks_roster . . . . . : 0x%lx", ptr_server->iks_roster);
        weechat_log_printf ("  iks_features . . . . : %d",    ptr_server->iks_features);
        weechat_log_printf ("  iks_authorized . . . : %d",    ptr_server->iks_authorized);
        weechat_log_printf ("  hook_connect . . . . : 0x%lx", ptr_server->hook_connect);
        weechat_log_printf ("  hook_fd. . . . . . . : 0x%lx", ptr_server->hook_fd);
        weechat_log_printf ("  is_connected . . . . : %d",    ptr_server->is_connected);
        weechat_log_printf ("  tls_connected. . . . : %d",    ptr_server->tls_connected);
#ifdef HAVE_GNUTLS
        weechat_log_printf ("  gnutls_sess. . . . . : 0x%lx", ptr_server->gnutls_sess);
#endif
        weechat_log_printf ("  reconnect_start. . . : %ld",   ptr_server->reconnect_start);
        weechat_log_printf ("  command_time . . . . : %ld",   ptr_server->command_time);
        weechat_log_printf ("  reconnect_join . . . : %d",    ptr_server->reconnect_join);
        weechat_log_printf ("  disable_autojoin . . : %d",    ptr_server->disable_autojoin);
        weechat_log_printf ("  is_away. . . . . . . : %d",    ptr_server->is_away);
        weechat_log_printf ("  away_message . . . . : '%s'",  ptr_server->away_message);
        weechat_log_printf ("  away_time. . . . . . : %ld",   ptr_server->away_time);
        weechat_log_printf ("  lag. . . . . . . . . : %d",    ptr_server->lag);
        weechat_log_printf ("  lag_check_time . . . : tv_sec:%d, tv_usec:%d",
                            ptr_server->lag_check_time.tv_sec,
                            ptr_server->lag_check_time.tv_usec);
        weechat_log_printf ("  lag_next_check . . . : %ld",   ptr_server->lag_next_check);
        weechat_log_printf ("  buffer . . . . . . . : 0x%lx", ptr_server->buffer);
        weechat_log_printf ("  buffer_as_string . . : '%s'",  ptr_server->buffer_as_string);
        weechat_log_printf ("  buddies_count. . . . : %d",    ptr_server->buddies_count);
        weechat_log_printf ("  buddies. . . . . . . : 0x%lx", ptr_server->buddies);
        weechat_log_printf ("  last_buddy . . . . . : 0x%lx", ptr_server->last_buddy);
        weechat_log_printf ("  mucs . . . . . . . . : 0x%lx", ptr_server->mucs);
        weechat_log_printf ("  last_muc . . . . . . : 0x%lx", ptr_server->last_muc);
        weechat_log_printf ("  prev_server. . . . . : 0x%lx", ptr_server->prev_server);
        weechat_log_printf ("  next_server. . . . . : 0x%lx", ptr_server->next_server);
        
        for (ptr_buddy = ptr_server->buddies; ptr_buddy;
             ptr_buddy = ptr_buddy->next_buddy)
        {
            jabber_buddy_print_log (ptr_buddy);
        }
        
        for (ptr_muc = ptr_server->mucs; ptr_muc;
             ptr_muc = ptr_muc->next_muc)
        {
            jabber_muc_print_log (ptr_muc);
        }
    }
}
