/*
 * relay-remote.c - remote relay server functions for relay plugin
 *
 * Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <gnutls/gnutls.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-config.h"
#include "relay-remote.h"
#include "relay-websocket.h"
#ifdef HAVE_CJSON
#include "api/remote/relay-remote-event.h"
#include "api/remote/relay-remote-network.h"
#endif


char *relay_remote_option_string[RELAY_REMOTE_NUM_OPTIONS] =
{ "url", "autoconnect", "autoreconnect_delay", "proxy", "tls_verify",
  "password", "totp_secret" };
char *relay_remote_option_default[RELAY_REMOTE_NUM_OPTIONS] =
{ "", "off", "10", "", "on", "", "" };

struct t_relay_remote *relay_remotes = NULL;
struct t_relay_remote *last_relay_remote = NULL;
int relay_remotes_count = 0;            /* number of remotes                */

struct t_relay_remote *relay_remotes_temp = NULL;     /* first temp. remote */
struct t_relay_remote *last_relay_remote_temp = NULL; /* last temp. remote  */


/*
 * Searches for a remote option name.
 *
 * Returns index of option in enum t_relay_remote_option, -1 if not found.
 */

int
relay_remote_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        if (strcmp (relay_remote_option_string[i], option_name) == 0)
            return i;
    }

    /* remote option not found */
    return -1;
}

/*
 * Checks if a remote pointer is valid.
 *
 * Returns:
 *   1: remote exists
 *   0: remote does not exist
 */

int
relay_remote_valid (struct t_relay_remote *remote)
{
    struct t_relay_remote *ptr_remote;

    if (!remote)
        return 0;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (ptr_remote == remote)
            return 1;
    }

    /* remote not found */
    return 0;
}

/*
 * Searches for a remote by name.
 *
 * Returns pointer to remote found, NULL if not found.
 */

struct t_relay_remote *
relay_remote_search (const char *name)
{
    struct t_relay_remote *ptr_remote;

    if (!name || !name[0])
        return NULL;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (strcmp (ptr_remote->name, name) == 0)
            return ptr_remote;
    }

    /* remote not found */
    return NULL;
}

/*
 * Searches for a remote by number (first remote is 0).
 *
 * Returns pointer to remote found, NULL if not found.
 */

struct t_relay_remote *
relay_remote_search_by_number (int number)
{
    struct t_relay_remote *ptr_remote;
    int i;

    i = 0;
    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (i == number)
            return ptr_remote;
        i++;
    }

    /* remote not found */
    return NULL;
}

/*
 * Checks if a remote name is valid: it must contain only alphabetic chars
 * or digits.
 *
 * Returns:
 *   1: name is valid
 *   0: name is invalid
 */

int
relay_remote_name_valid (const char *name)
{
    const char *ptr_name;

    if (!name || !name[0])
        return 0;

    ptr_name = name;
    while (ptr_name[0])
    {
        if (!isalnum ((unsigned char)ptr_name[0]))
            return 0;
        ptr_name++;
    }

    /* name is valid */
    return 1;
}

/*
 * Extracts TLS, address and port from remote URL.
 *
 * If address is an IPv6 like "[::1]", the square brackets are removed.
 *
 * Returns:
 *   1: OK
 *   0: error, invalid URL
 */

int
relay_remote_parse_url (const char *url,
                        int *tls, char **address, int *port)
{
    const char *ptr_url;
    char *pos, *str_port, *error;
    long number;

    if (tls)
        *tls = 0;
    if (address)
        *address = NULL;
    if (port)
        *port = RELAY_REMOTE_DEFAULT_PORT;

    if (!url || !url[0])
        return 0;

    /* check scheme and extract TLS flag */
    if (strncmp (url, "http://", 7) == 0)
    {
        ptr_url = url + 7;
    }
    else if (strncmp (url, "https://", 8) == 0)
    {
        if (tls)
            *tls = 1;
        ptr_url = url + 8;
    }
    else
    {
        return 0;
    }

    /* check if there is an IPv6 address with square brackets, like "[::1]" */
    if (ptr_url[0] == '[')
    {
        /* extract IPv6 address between square brackets */
        pos = strchr (ptr_url, ']');
        if (!pos)
            return 0;
        if (address)
            *address = weechat_strndup (ptr_url + 1, pos - ptr_url - 1);
        ptr_url = pos + 1;
    }
    else
    {
        /* extract another address */
        pos = strrchr (ptr_url, ':');
        if (!pos)
            pos = strchr (ptr_url, '/');
        if (!pos)
            pos = strchr (ptr_url, '?');
        if (address)
        {
            *address = (pos) ?
                weechat_strndup (ptr_url, pos - ptr_url) : strdup (ptr_url);
        }
    }

    /* extract port number */
    pos = strrchr (ptr_url, ':');
    if (pos)
    {
        ptr_url = pos + 1;
        pos = strchr (ptr_url, '/');
        if (!pos)
            pos = strchr (ptr_url, '?');
        str_port = (pos) ?
            weechat_strndup (ptr_url, pos - ptr_url) : strdup (ptr_url);
        if (!str_port)
            return 0;
        error = NULL;
        number = strtol (str_port, &error, 10);
        if (error && !error[0] && (number >= 0) && (number <= 65535))
        {
            if (port)
                *port = number;
            free (str_port);
        }
        else
        {
            free (str_port);
            return 0;
        }
    }

    return 1;
}

/*
 * Checks if a remote URL is valid.
 *
 * Returns:
 *   1: URL is valid
 *   0: URL is invalid
 */

int
relay_remote_url_valid (const char *url)
{
    return relay_remote_parse_url (url, NULL, NULL, NULL);
}

/*
 * Sends a signal with the status of remote ("relay_remote_xxx").
 */

void
relay_remote_send_signal (struct t_relay_remote *remote)
{
    char signal[128];

    snprintf (signal, sizeof (signal),
              "relay_remote_%s",
              relay_status_name[remote->status]);
    weechat_hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_POINTER, remote);
}

/*
 * Allocates and initializes new remote structure.
 *
 * Returns pointer to new remote, NULL if error.
 */

struct t_relay_remote *
relay_remote_alloc (const char *name)
{
    struct t_relay_remote *new_remote;
    int i;

    if (!relay_remote_name_valid (name))
        return NULL;

    if (relay_remote_search (name))
        return NULL;

    new_remote = malloc (sizeof (*new_remote));
    if (!new_remote)
        return NULL;

    new_remote->name = strdup (name);
    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        new_remote->options[i] = NULL;
    }
    new_remote->address = NULL;
    new_remote->port = 0;
    new_remote->tls = 0;
    new_remote->status = RELAY_STATUS_DISCONNECTED;
    new_remote->password_hash_algo = -1;
    new_remote->password_hash_iterations = -1;
    new_remote->totp = -1;
    new_remote->websocket_key = NULL;
    new_remote->sock = -1;
    new_remote->hook_url_handshake = NULL;
    new_remote->hook_connect = NULL;
    new_remote->hook_fd = NULL;
    new_remote->gnutls_sess = NULL;
    new_remote->ws_deflate = relay_websocket_deflate_alloc ();
    new_remote->version_ok = 0;
    new_remote->synced = 0;
    new_remote->partial_ws_frame = NULL;
    new_remote->partial_ws_frame_size = 0;
    new_remote->reconnect_delay = 0;
    new_remote->reconnect_start = 0;
    new_remote->prev_remote = NULL;
    new_remote->next_remote = NULL;

    return new_remote;
}

/*
 * Searches for position of remote in list (to keep remotes sorted by name).
 */

struct t_relay_remote *
relay_remote_find_pos (struct t_relay_remote *remote,
                       struct t_relay_remote *list_remotes)
{
    struct t_relay_remote *ptr_remote;

    for (ptr_remote = list_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (weechat_strcmp (remote->name, ptr_remote->name) < 0)
            return ptr_remote;
    }

    /* position not found */
    return NULL;
}

/*
 * Adds a remote in a linked list.
 */

void
relay_remote_add (struct t_relay_remote *remote,
                  struct t_relay_remote **list_remotes,
                  struct t_relay_remote **last_list_remote)
{
    struct t_relay_remote *pos_remote;

    pos_remote = relay_remote_find_pos (remote, *list_remotes);
    if (pos_remote)
    {
        /* add remote before "pos_remote" */
        remote->prev_remote = pos_remote->prev_remote;
        remote->next_remote = pos_remote;
        if (pos_remote->prev_remote)
            (pos_remote->prev_remote)->next_remote = remote;
        else
            *list_remotes = remote;
        pos_remote->prev_remote = remote;
    }
    else
    {
        /* add remote to end of list */
        remote->prev_remote = *last_list_remote;
        remote->next_remote = NULL;
        if (*last_list_remote)
            (*last_list_remote)->next_remote = remote;
        else
            *list_remotes = remote;
        *last_list_remote = remote;
    }
}

/*
 * Sets URL in a remote.
 */

void
relay_remote_set_url (struct t_relay_remote *remote, const char *url)
{
    free (remote->address);
    relay_remote_parse_url (url, &remote->tls, &remote->address, &remote->port);
}

/*
 * Creates a new remote with options.
 *
 * Returns pointer to new remote, NULL if error.
 */

struct t_relay_remote *
relay_remote_new_with_options (const char *name, struct t_config_option **options)
{
    struct t_relay_remote *new_remote;
    int i;

    new_remote = relay_remote_alloc (name);
    if (!new_remote)
        return NULL;

    if (!relay_remote_url_valid (weechat_config_string (options[RELAY_REMOTE_OPTION_URL])))
    {
        free (new_remote);
        return NULL;
    }

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        new_remote->options[i] = options[i];
    }
    relay_remote_add (new_remote, &relay_remotes, &last_relay_remote);
    relay_remote_set_url (
        new_remote,
        weechat_config_string (new_remote->options[RELAY_REMOTE_OPTION_URL]));

    relay_remotes_count++;

    relay_remote_send_signal (new_remote);

    return new_remote;
}

/*
 * Creates a new remote.
 *
 * Returns pointer to new remote, NULL if error.
 */

struct t_relay_remote *
relay_remote_new (const char *name,
                  const char *url,
                  const char *autoconnect,
                  const char *autoreconnect_delay,
                  const char *proxy,
                  const char *tls_verify,
                  const char *password,
                  const char *totp_secret)
{
    struct t_config_option *option[RELAY_REMOTE_NUM_OPTIONS];
    const char *value[RELAY_REMOTE_NUM_OPTIONS];
    struct t_relay_remote *new_remote;
    int i;

    if (!name || !name[0] || !url || !url[0])
        return NULL;

    value[RELAY_REMOTE_OPTION_URL] = url;
    value[RELAY_REMOTE_OPTION_AUTOCONNECT] = autoconnect;
    value[RELAY_REMOTE_OPTION_AUTORECONNECT_DELAY] = autoreconnect_delay;
    value[RELAY_REMOTE_OPTION_PROXY] = proxy;
    value[RELAY_REMOTE_OPTION_TLS_VERIFY] = tls_verify;
    value[RELAY_REMOTE_OPTION_PASSWORD] = password;
    value[RELAY_REMOTE_OPTION_TOTP_SECRET] = totp_secret;

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        option[i] = relay_config_create_remote_option (
            name,
            i,
            (value[i]) ? value[i] : relay_remote_option_default[i]);
    }

    new_remote = relay_remote_new_with_options (name, option);
    if (!new_remote)
    {
        for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
        {
            weechat_config_option_free (option[i]);
        }
    }

    return new_remote;
}

/*
 * Creates a new remote using an infolist.
 *
 * This is called to restore remotes after /upgrade.
 */

struct t_relay_remote *
relay_remote_new_with_infolist (struct t_infolist *infolist)
{
    struct t_relay_remote *new_remote;
    Bytef *ptr_dict;
    int dict_size, ws_frame_size;
    void *ptr_ws_frame;

    new_remote = malloc (sizeof (*new_remote));
    if (!new_remote)
        return NULL;

    new_remote->name = strdup (weechat_infolist_string (infolist, "name"));
    new_remote->address = strdup (weechat_infolist_string (infolist, "address"));
    new_remote->port = weechat_infolist_integer (infolist, "port");
    new_remote->tls = weechat_infolist_integer (infolist, "tls");
    new_remote->status = weechat_infolist_integer (infolist, "status");
    new_remote->password_hash_algo = weechat_infolist_integer (
        infolist, "password_hash_algo");
    new_remote->password_hash_iterations = weechat_infolist_integer (
        infolist, "password_hash_iterations");
    new_remote->totp = weechat_infolist_integer (infolist, "totp");
    new_remote->websocket_key = strdup (weechat_infolist_string (infolist, "websocket_key"));
    new_remote->sock = weechat_infolist_integer (infolist, "sock");
    new_remote->hook_url_handshake = NULL;
    new_remote->hook_connect = NULL;
    new_remote->hook_fd = NULL;
    new_remote->gnutls_sess = NULL;
    new_remote->ws_deflate = relay_websocket_deflate_alloc ();
    new_remote->ws_deflate->enabled = weechat_infolist_integer (infolist, "ws_deflate_enabled");
    new_remote->ws_deflate->server_context_takeover = weechat_infolist_integer (infolist, "ws_deflate_server_context_takeover");
    new_remote->ws_deflate->client_context_takeover = weechat_infolist_integer (infolist, "ws_deflate_client_context_takeover");
    new_remote->ws_deflate->window_bits_deflate = weechat_infolist_integer (infolist, "ws_deflate_window_bits_deflate");
    new_remote->ws_deflate->window_bits_inflate = weechat_infolist_integer (infolist, "ws_deflate_window_bits_inflate");
    new_remote->ws_deflate->server_max_window_bits_recv = weechat_infolist_integer (infolist, "ws_deflate_server_max_window_bits_recv");
    new_remote->ws_deflate->client_max_window_bits_recv = weechat_infolist_integer (infolist, "ws_deflate_client_max_window_bits_recv");
    new_remote->ws_deflate->strm_deflate = NULL;
    new_remote->ws_deflate->strm_inflate = NULL;
    if (weechat_infolist_search_var (infolist, "ws_deflate_strm_deflate_dict"))
    {
        ptr_dict = weechat_infolist_buffer (infolist, "ws_deflate_strm_deflate_dict", &dict_size);
        if (ptr_dict)
        {
            new_remote->ws_deflate->strm_deflate = calloc (1, sizeof (*new_remote->ws_deflate->strm_deflate));
            if (new_remote->ws_deflate->strm_deflate)
            {
                if (relay_websocket_deflate_init_stream_deflate (new_remote->ws_deflate))
                {
                    deflateSetDictionary (new_remote->ws_deflate->strm_deflate,
                                          ptr_dict, dict_size);
                }
            }
        }
    }
    if (weechat_infolist_search_var (infolist, "ws_deflate_strm_inflate_dict"))
    {
        ptr_dict = weechat_infolist_buffer (infolist, "ws_deflate_strm_inflate_dict", &dict_size);
        if (ptr_dict)
        {
            new_remote->ws_deflate->strm_inflate = calloc (1, sizeof (*new_remote->ws_deflate->strm_inflate));
            if (new_remote->ws_deflate->strm_inflate)
            {
                if (relay_websocket_deflate_init_stream_inflate (new_remote->ws_deflate))
                {
                    inflateSetDictionary (new_remote->ws_deflate->strm_inflate,
                                          ptr_dict, dict_size);
                }
            }
        }
    }
    new_remote->version_ok = weechat_infolist_integer (infolist, "version_ok");
    new_remote->synced = weechat_infolist_integer (infolist, "synced");
    new_remote->reconnect_delay = weechat_infolist_integer (infolist, "reconnect_delay");
    new_remote->reconnect_start = weechat_infolist_integer (infolist, "reconnect_start");
    ptr_ws_frame = weechat_infolist_buffer (infolist, "partial_ws_frame", &ws_frame_size);
    if (ptr_ws_frame && (ws_frame_size > 0))
    {
        new_remote->partial_ws_frame = malloc (ws_frame_size);
        if (new_remote->partial_ws_frame)
        {
            memcpy (new_remote->partial_ws_frame, ptr_ws_frame, ws_frame_size);
            new_remote->partial_ws_frame_size = ws_frame_size;
        }
    }
    new_remote->prev_remote = NULL;
    new_remote->next_remote = relay_remotes;
    if (relay_remotes)
        relay_remotes->prev_remote = new_remote;
    else
        last_relay_remote = new_remote;
    relay_remotes = new_remote;

    relay_remotes_count++;

    return new_remote;
}

/*
 * Sets status for a remote.
 */

void
relay_remote_set_status (struct t_relay_remote *remote,
                         enum t_relay_status status)
{
    /*
     * IMPORTANT: if changes are made in this function or sub-functions called,
     * please also update the function relay_remote_add_to_infolist:
     * when the flag force_disconnected_state is set to 1 we simulate
     * a disconnected state for remote in infolist (used on /upgrade -save)
     */

    if (remote->status == status)
        return;

    remote->status = status;

    relay_remote_send_signal (remote);
    weechat_bar_item_update ("input_prompt");
}

/*
 * Connects to a remote WeeChat relay/api.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_connect (struct t_relay_remote *remote)
{
    if (!remote)
        return 0;

#ifdef HAVE_CJSON
    return relay_remote_network_connect (remote);
#else
    weechat_printf (NULL,
                    _("%s%s: error: unable to connect to a remote relay via API "
                      "(cJSON support is not enabled)"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME);
    return 0;
#endif /* HAVE_CJSON */
}

/*
 * Callback for auto-connect to remotes (called at startup).
 */

int
relay_remote_auto_connect_timer_cb (const void *pointer, void *data,
                                    int remaining_calls)
{
    struct t_relay_remote *ptr_remote;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        if (weechat_config_boolean (ptr_remote->options[RELAY_REMOTE_OPTION_AUTOCONNECT]))
            relay_remote_connect (ptr_remote);
    }

    return WEECHAT_RC_OK;
}

/*
 * Auto-connects to all remotes with option autoconnect to "on".
 */

void
relay_remote_auto_connect (void)
{
    weechat_hook_timer (1, 0, 1,
                        &relay_remote_auto_connect_timer_cb, NULL, NULL);
}

/*
 * Sends JSON data to a remote WeeChat relay/api.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_send (struct t_relay_remote *remote, const char *json)
{
#ifdef HAVE_CJSON
    if (!remote || (remote->status != RELAY_STATUS_CONNECTED) || !json)
        return 0;

    return (relay_remote_network_send (remote, RELAY_MSG_STANDARD,
                                       json, strlen (json)) > 0) ?
        1 : 0;
#else
    (void) remote;
    (void) json;
    return 0;
#endif /* HAVE_CJSON */
}

/*
 * Renames a remote.
 *
 * Returns:
 *   1: OK
 *   0: error (remote not renamed)
 */

int
relay_remote_rename (struct t_relay_remote *remote, const char *name)
{
    int length, i;
    char *option_name;

    if (!remote || !name || !name[0] || !relay_remote_name_valid (name)
        || relay_remote_search (name))
    {
        return 0;
    }

    length = strlen (name) + 64;
    option_name = malloc (length);
    if (!option_name)
        return 0;

    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        if (remote->options[i])
        {
            snprintf (option_name, length,
                      "%s.%s",
                      name,
                      relay_remote_option_string[i]);
            weechat_config_option_rename (remote->options[i], option_name);
        }
    }

    free (remote->name);
    remote->name = strdup (name);

    free (option_name);

    /* re-insert remote in list (for sorting remotes by name) */
    if (remote->prev_remote)
        (remote->prev_remote)->next_remote = remote->next_remote;
    else
        relay_remotes = remote->next_remote;
    if (remote->next_remote)
        (remote->next_remote)->prev_remote = remote->prev_remote;
    else
        last_relay_remote = remote->prev_remote;
    relay_remote_add (remote, &relay_remotes, &last_relay_remote);

    return 1;
}

/*
 * Disconnects one remote.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_disconnect (struct t_relay_remote *remote)
{
    if (!remote)
        return 0;

#ifdef HAVE_CJSON
    if (remote->sock >= 0)
        relay_remote_network_disconnect (remote);
    return 1;
#else
    return 0;
#endif /* HAVE_CJSON */
}

/*
 * Schedules reconnection to remote.
 */

void
relay_remote_reconnect_schedule (struct t_relay_remote *remote)
{
    int minutes, seconds;

    if (weechat_config_integer (remote->options[RELAY_REMOTE_OPTION_AUTORECONNECT_DELAY]) == 0)
    {
        remote->reconnect_delay = 0;
        remote->reconnect_start = 0;
        return;
    }

    /* growing reconnect delay */
    if (remote->reconnect_delay == 0)
        remote->reconnect_delay = weechat_config_integer (remote->options[RELAY_REMOTE_OPTION_AUTORECONNECT_DELAY]);
    else
        remote->reconnect_delay = remote->reconnect_delay * weechat_config_integer (relay_config_api_remote_autoreconnect_delay_growing);
    if ((weechat_config_integer (relay_config_api_remote_autoreconnect_delay_max) > 0)
        && (remote->reconnect_delay > weechat_config_integer (relay_config_api_remote_autoreconnect_delay_max)))
    {
        remote->reconnect_delay = weechat_config_integer (relay_config_api_remote_autoreconnect_delay_max);
    }

    remote->reconnect_start = time (NULL);

    minutes = remote->reconnect_delay / 60;
    seconds = remote->reconnect_delay % 60;
    if ((minutes > 0) && (seconds > 0))
    {
        weechat_printf (
            NULL,
            _("remote[%s]: reconnecting to remote relay in %d %s, %d %s"),
            remote->name,
            minutes,
            NG_("minute", "minutes", minutes),
            seconds,
            NG_("second", "seconds", seconds));
    }
    else if (minutes > 0)
    {
        weechat_printf (
            NULL,
            _("remote[%s]: reconnecting to remote relay in %d %s"),
            remote->name,
            minutes,
            NG_("minute", "minutes", minutes));
    }
    else
    {
        weechat_printf (
            NULL,
            _("remote[%s]: reconnecting to remote relay in %d %s"),
            remote->name,
            seconds,
            NG_("second", "seconds", seconds));
    }
}

/*
 * Reconnects to a remote WeeChat relay/api.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_reconnect (struct t_relay_remote *remote)
{
    int rc;

    if (!remote)
        return 0;

    remote->reconnect_start = 0;

#ifdef HAVE_CJSON
    if (!relay_remote_disconnect (remote))
        return 0;
    rc = relay_remote_network_connect (remote);
    if (!rc)
        relay_remote_reconnect_schedule (remote);
    return rc;
#else
    (void) rc;
    weechat_printf (NULL,
                    _("%s%s: error: unable to connect to a remote relay via API "
                      "(cJSON support is not enabled)"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME);
    return 0;
#endif /* HAVE_CJSON */
}

/*
 * Timer used to auto-reconnect to remotes.
 */

void
relay_remote_timer (void)
{
    struct t_relay_remote *ptr_remote, *ptr_next_remote;
    time_t current_time;

    current_time = time (NULL);

    ptr_remote = relay_remotes;
    while (ptr_remote)
    {
        ptr_next_remote = ptr_remote->next_remote;

        /* check if reconnection is pending */
        if ((ptr_remote->sock <= 0)
            && (ptr_remote->reconnect_start > 0)
            && (current_time >= (ptr_remote->reconnect_start + ptr_remote->reconnect_delay)))
        {
            relay_remote_reconnect (ptr_remote);
        }

        ptr_remote = ptr_next_remote;
    }
}

/*
 * Disconnects all remotes.
 */

void
relay_remote_disconnect_all (void)
{
    struct t_relay_remote *ptr_remote;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        relay_remote_disconnect (ptr_remote);
    }
}

/*
 * Callback for input data in a remote buffer.
 */

void
relay_remote_buffer_input (struct t_gui_buffer *buffer, const char *input_data)
{
#ifdef HAVE_CJSON
    relay_remote_event_buffer_input (buffer, input_data);
#else
    (void) buffer;
    (void) input_data;
#endif
}

/*
 * Deletes a remote.
 */

void
relay_remote_free (struct t_relay_remote *remote)
{
    int i;

    if (!remote)
        return;

    /* remove remote from list */
    if (remote->prev_remote)
        (remote->prev_remote)->next_remote = remote->next_remote;
    if (remote->next_remote)
        (remote->next_remote)->prev_remote = remote->prev_remote;
    if (relay_remotes == remote)
        relay_remotes = remote->next_remote;
    if (last_relay_remote == remote)
        last_relay_remote = remote->prev_remote;

    /* free data */
    free (remote->name);
    for (i = 0; i < RELAY_REMOTE_NUM_OPTIONS; i++)
    {
        weechat_config_option_free (remote->options[i]);
    }
    free (remote->address);
    free (remote->websocket_key);
    weechat_unhook (remote->hook_url_handshake);
    weechat_unhook (remote->hook_connect);
    weechat_unhook (remote->hook_fd);
    relay_websocket_deflate_free (remote->ws_deflate);
    free (remote->partial_ws_frame);

    free (remote);

    relay_remotes_count--;
}

/*
 * Removes all remotes.
 */

void
relay_remote_free_all (void)
{
    while (relay_remotes)
    {
        relay_remote_free (relay_remotes);
    }
}

/*
 * Adds a remote in an infolist.
 *
 * If force_disconnected_state == 1, the infolist contains the remote
 * in a disconnected state (but the remote is unchanged, still connected if it
 * was).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_remote_add_to_infolist (struct t_infolist *infolist,
                              struct t_relay_remote *remote,
                              int force_disconnected_state)
{
    struct t_infolist_item *ptr_item;
    Bytef *dict;
    uInt dict_size;

    if (!infolist || !remote)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", remote->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "address", remote->address))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "port", remote->port))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "tls", remote->tls))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "password_hash_algo", remote->password_hash_algo))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "password_hash_iterations", remote->password_hash_iterations))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "totp", remote->totp))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "websocket_key", remote->websocket_key))
        return 0;
    if (!RELAY_STATUS_HAS_ENDED(remote->status) && force_disconnected_state)
    {
        if (!weechat_infolist_new_var_integer (ptr_item, "status", RELAY_STATUS_DISCONNECTED))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "sock", -1))
            return 0;
        if (!weechat_infolist_new_var_buffer (ptr_item, "partial_ws_frame", NULL, 0))
            return 0;
    }
    else
    {
        if (!weechat_infolist_new_var_integer (ptr_item, "status", remote->status))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "sock", remote->sock))
            return 0;
        if (!weechat_infolist_new_var_buffer (ptr_item, "partial_ws_frame", remote->partial_ws_frame, remote->partial_ws_frame_size))
            return 0;
    }
    if (remote->ws_deflate->strm_deflate || remote->ws_deflate->strm_inflate)
    {
        /* save the deflate/inflate dictionary, as it's required after /upgrade */
        dict = malloc (32768);
        if (dict)
        {
            if (remote->ws_deflate->strm_deflate)
            {
                if (deflateGetDictionary (remote->ws_deflate->strm_deflate, dict, &dict_size) == Z_OK)
                {
                    weechat_infolist_new_var_buffer (ptr_item,
                                                     "ws_deflate_strm_deflate_dict",
                                                     dict, dict_size);
                }
            }
            if (remote->ws_deflate->strm_inflate)
            {
                if (inflateGetDictionary (remote->ws_deflate->strm_inflate, dict, &dict_size) == Z_OK)
                {
                    weechat_infolist_new_var_buffer (ptr_item,
                                                     "ws_deflate_strm_inflate_dict",
                                                     dict, dict_size);
                }
            }
            free (dict);
        }
    }
    if (!weechat_infolist_new_var_integer (ptr_item, "version_ok", remote->version_ok))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "reconnect_delay", remote->reconnect_delay))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "reconnect_start", remote->reconnect_start))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "synced", remote->synced))
        return 0;

    return 1;
}

/*
 * Prints remotes in WeeChat log file (usually for crash dump).
 */

void
relay_remote_print_log (void)
{
    struct t_relay_remote *ptr_remote;

    for (ptr_remote = relay_remotes; ptr_remote;
         ptr_remote = ptr_remote->next_remote)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[relay remote (addr:%p)]", ptr_remote);
        weechat_log_printf ("  name. . . . . . . . . . : '%s'", ptr_remote->name);
        weechat_log_printf ("  url . . . . . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_URL]));
        weechat_log_printf ("  autoconnect . . . . . . : %s",
                            (weechat_config_boolean (ptr_remote->options[RELAY_REMOTE_OPTION_AUTOCONNECT])) ?
                            "on" : "off");
        weechat_log_printf ("  autoreconnect_delay . . : %d",
                            weechat_config_integer (ptr_remote->options[RELAY_REMOTE_OPTION_AUTORECONNECT_DELAY]));
        weechat_log_printf ("  proxy . . . . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_PROXY]));
        weechat_log_printf ("  tls_verify. . . . . . . : %s",
                            (weechat_config_boolean (ptr_remote->options[RELAY_REMOTE_OPTION_TLS_VERIFY])) ?
                            "on" : "off");
        weechat_log_printf ("  password. . . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_PASSWORD]));
        weechat_log_printf ("  totp_secret . . . . . . : '%s'",
                            weechat_config_string (ptr_remote->options[RELAY_REMOTE_OPTION_TOTP_SECRET]));
        weechat_log_printf ("  address . . . . . . . . : '%s'", ptr_remote->address);
        weechat_log_printf ("  port. . . . . . . . . . : %d", ptr_remote->port);
        weechat_log_printf ("  tls . . . . . . . . . . : %d", ptr_remote->tls);
        weechat_log_printf ("  status. . . . . . . . . : %d (%s)",
                            ptr_remote->status,
                            relay_status_string[ptr_remote->status]);
        weechat_log_printf ("  password_hash_algo. . . : %d", ptr_remote->password_hash_algo);
        weechat_log_printf ("  password_hash_iterations: %d", ptr_remote->password_hash_iterations);
        weechat_log_printf ("  totp. . . . . . . . . . : %d", ptr_remote->totp);
        weechat_log_printf ("  websocket_key . . . . . : %p", ptr_remote->websocket_key);
        weechat_log_printf ("  sock. . . . . . . . . . : %d", ptr_remote->sock);
        weechat_log_printf ("  hook_url_handshake. . . : %p", ptr_remote->hook_url_handshake);
        weechat_log_printf ("  hook_connect. . . . . . : %p", ptr_remote->hook_connect);
        weechat_log_printf ("  hook_fd . . . . . . . . : %p", ptr_remote->hook_fd);
        weechat_log_printf ("  gnutls_sess . . . . . . : %p", ptr_remote->gnutls_sess);
        relay_websocket_deflate_print_log (ptr_remote->ws_deflate, "");
        weechat_log_printf ("  version_ok. . . . . . . : %d", ptr_remote->version_ok);
        weechat_log_printf ("  reconnect_delay . . . . : %d", ptr_remote->reconnect_delay);
        weechat_log_printf ("  reconnect_start . . . . : %d", ptr_remote->reconnect_start);
        weechat_log_printf ("  synced. . . . . . . . . : %d", ptr_remote->synced);
        weechat_log_printf ("  partial_ws_frame. . . . : %p (%d bytes)",
                            ptr_remote->partial_ws_frame,
                            ptr_remote->partial_ws_frame_size);
        weechat_log_printf ("  prev_remote . . . . . . : %p", ptr_remote->prev_remote);
        weechat_log_printf ("  next_remote . . . . . . : %p", ptr_remote->next_remote);
    }
}
