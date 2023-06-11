/*
 * relay-client.c - client functions for relay plugin
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
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <gnutls/gnutls.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"
#include "relay-auth.h"
#include "relay-config.h"
#include "relay-buffer.h"
#include "relay-network.h"
#include "relay-raw.h"
#include "relay-server.h"
#include "relay-websocket.h"
#include "irc/relay-irc.h"
#include "weechat/relay-weechat.h"


char *relay_client_status_string[] =   /* status strings for display        */
{ N_("connecting"), N_("waiting auth"),
  N_("connected"), N_("auth failed"), N_("disconnected")
};
char *relay_client_status_name[] =     /* name of status (for signal/info)  */
{ "connecting", "waiting_auth",
  "connected", "auth_failed", "disconnected"
};

char *relay_client_data_type_string[] = /* strings for data types           */
{ "text", "binary" };

char *relay_client_msg_type_string[] = /* prefix in raw buffer for message  */
{ "", "[PING]\n", "[PONG]\n", "[CLOSE]\n" };

struct t_relay_client *relay_clients = NULL;
struct t_relay_client *last_relay_client = NULL;
int relay_client_count = 0;            /* number of clients                 */


/*
 * Checks if a client pointer is valid.
 *
 * Returns:
 *   1: client exists
 *   0: client does not exist
 */

int
relay_client_valid (struct t_relay_client *client)
{
    struct t_relay_client *ptr_client;

    if (!client)
        return 0;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (ptr_client == client)
            return 1;
    }

    /* client not found */
    return 0;
}

/*
 * Searches for a client by number (first client is 0).
 *
 * Returns pointer to client found, NULL if not found.
 */

struct t_relay_client *
relay_client_search_by_number (int number)
{
    struct t_relay_client *ptr_client;
    int i;

    i = 0;
    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (i == number)
            return ptr_client;
        i++;
    }

    /* client not found */
    return NULL;
}

/*
 * Searches for a client by id.
 *
 * Returns pointer to client found, NULL if not found.
 */

struct t_relay_client *
relay_client_search_by_id (int id)
{
    struct t_relay_client *ptr_client;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if (ptr_client->id == id)
            return ptr_client;
    }

    /* client not found */
    return NULL;
}

/*
 * Searches for a client status.
 *
 * Returns index of status in enum t_relay_status, -1 if status is not found.
 */

int
relay_client_status_search (const char *name)
{
    int i;

    if (!name)
        return -1;

    for (i = 0; i < RELAY_NUM_STATUS; i++)
    {
        if (strcmp (relay_client_status_name[i], name) == 0)
            return i;
    }

    /* status not found */
    return -1;
}

/*
 * Returns the number of active clients (connecting or connected) on a given
 * server port.
 */

int
relay_client_count_active_by_port (int server_port)
{
    struct t_relay_client *ptr_client;
    int count;

    count = 0;
    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        if ((ptr_client->server_port == server_port)
            && !RELAY_CLIENT_HAS_ENDED(ptr_client))
        {
            count++;
        }
    }

    return count;
}

/*
 * Sends a signal with the status of client ("relay_client_xxx").
 */

void
relay_client_send_signal (struct t_relay_client *client)
{
    char signal[128];

    snprintf (signal, sizeof (signal),
              "relay_client_%s",
              relay_client_status_name[client->status]);
    weechat_hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_POINTER, client);
}

/*
 * Sets description for a client.
 */

void
relay_client_set_desc (struct t_relay_client *client)
{
    char desc[512];

    if (client->desc)
        free (client->desc);

    snprintf (desc, sizeof (desc),
              "%d/%s%s%s%s/%s%s%s%s",
              client->id,
              (client->tls) ? "tls." : "",
              relay_protocol_string[client->protocol],
              (client->protocol_args) ? "." : "",
              (client->protocol_args) ? client->protocol_args : "",
              client->address,
              (client->real_ip) ? "(" : "",
              (client->real_ip) ? client->real_ip : "",
              (client->real_ip) ? ")" : "");

    client->desc = strdup (desc);
}

/*
 * Timer callback for handshake with client (for TLS connection only).
 */

int
relay_client_handshake_timer_cb (const void *pointer, void *data,
                                 int remaining_calls)
{
    struct t_relay_client *client;
    int rc;

    /* make C compiler happy */
    (void) data;

    client = (struct t_relay_client *)pointer;

    rc = gnutls_handshake (client->gnutls_sess);

    if (rc == GNUTLS_E_SUCCESS)
    {
        /* handshake OK, set status to "connected" */
        weechat_unhook (client->hook_timer_handshake);
        client->hook_timer_handshake = NULL;
        client->gnutls_handshake_ok = 1;
        switch (client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_client_set_status (
                    client,
                    relay_weechat_get_initial_status (client));
                break;
            case RELAY_PROTOCOL_IRC:
                relay_client_set_status (
                    client,
                    relay_irc_get_initial_status (client));
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
        return WEECHAT_RC_OK;
    }

    if (gnutls_error_is_fatal (rc))
    {
        /* handshake error, disconnect client */
        weechat_printf_date_tags (
            NULL, 0, "relay_client",
            _("%s%s: TLS handshake failed for client %s%s%s: error %d %s"),
            weechat_prefix ("error"),
            RELAY_PLUGIN_NAME,
            RELAY_COLOR_CHAT_CLIENT,
            client->desc,
            RELAY_COLOR_CHAT,
            rc,
            gnutls_strerror (rc));
        weechat_unhook (client->hook_timer_handshake);
        client->hook_timer_handshake = NULL;
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
        return WEECHAT_RC_OK;
    }

    if (remaining_calls == 0)
    {
        /* handshake timeout, disconnect client */
        weechat_printf_date_tags (
            NULL, 0, "relay_client",
            _("%s%s: TLS handshake timeout for client %s%s%s"),
            weechat_prefix ("error"),
            RELAY_PLUGIN_NAME,
            RELAY_COLOR_CHAT_CLIENT,
            client->desc,
            RELAY_COLOR_CHAT);
        weechat_unhook (client->hook_timer_handshake);
        client->hook_timer_handshake = NULL;
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
        return WEECHAT_RC_OK;
    }

    /* handshake in progress, we will try again on next call to timer */
    return WEECHAT_RC_OK;
}

/*
 * Reads text data from a client: splits data on '\n' and keeps a partial
 * message if data does not end with '\n'.
 */

void
relay_client_recv_text (struct t_relay_client *client, const char *data)
{
    char *new_partial, *raw_msg, **lines, *pos, *tmp, *handshake;
    const char *ptr_real_ip;
    int i, num_lines, length, rc;

    if (client->partial_message)
    {
        new_partial = realloc (client->partial_message,
                               strlen (client->partial_message) +
                               strlen (data) + 1);
        if (!new_partial)
            return;
        client->partial_message = new_partial;
        strcat (client->partial_message, data);
    }
    else
        client->partial_message = strdup (data);

    pos = strrchr (client->partial_message, '\n');
    if (pos)
    {
        /* print message in raw buffer */
        raw_msg = weechat_strndup (client->partial_message,
                                   pos - client->partial_message + 1);
        if (raw_msg)
        {
            relay_raw_print (client, RELAY_CLIENT_MSG_STANDARD,
                             RELAY_RAW_FLAG_RECV,
                             raw_msg, strlen (raw_msg) + 1);
            free (raw_msg);
        }

        pos[0] = '\0';

        lines = weechat_string_split (client->partial_message, "\n", NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0, &num_lines);
        if (lines)
        {
            for (i = 0; i < num_lines; i++)
            {
                /* remove final '\r' */
                length = strlen (lines[i]);
                if ((length > 0) && (lines[i][length - 1] == '\r'))
                    lines[i][length - 1] = '\0';

                /* if websocket is initializing */
                if (client->websocket == RELAY_CLIENT_WEBSOCKET_INITIALIZING)
                {
                    if (lines[i][0])
                    {
                        /* web socket is initializing, read HTTP headers */
                        relay_websocket_save_header (client, lines[i]);
                    }
                    else
                    {
                        /*
                         * empty line means that we have received all HTTP
                         * headers: then we check the validity of websocket, and
                         * if it is OK, we'll do the handshake and answer to the
                         * client
                         */
                        rc = relay_websocket_client_handshake_valid (client);
                        if (rc == 0)
                        {
                            /* handshake from client is valid */
                            handshake  = relay_websocket_build_handshake (client);
                            if (handshake)
                            {
                                relay_client_send (client,
                                                   RELAY_CLIENT_MSG_STANDARD,
                                                   handshake,
                                                   strlen (handshake), NULL);
                                free (handshake);
                                client->websocket = RELAY_CLIENT_WEBSOCKET_READY;
                            }
                        }
                        else
                        {
                            switch (rc)
                            {
                                case -1:
                                    relay_websocket_send_http (client,
                                                               "400 Bad Request");
                                    if (weechat_relay_plugin->debug >= 1)
                                    {
                                        weechat_printf_date_tags (
                                            NULL, 0, "relay_client",
                                            _("%s%s: invalid websocket "
                                              "handshake received for client "
                                              "%s%s%s"),
                                            weechat_prefix ("error"),
                                            RELAY_PLUGIN_NAME,
                                            RELAY_COLOR_CHAT_CLIENT,
                                            client->desc,
                                            RELAY_COLOR_CHAT);
                                    }
                                    break;
                                case -2:
                                    relay_websocket_send_http (client,
                                                               "403 Forbidden");
                                    if (weechat_relay_plugin->debug >= 1)
                                    {
                                        weechat_printf_date_tags (
                                            NULL, 0, "relay_client",
                                            _("%s%s: origin \"%s\" "
                                              "not allowed for websocket"),
                                            weechat_prefix ("error"),
                                            RELAY_PLUGIN_NAME,
                                            weechat_hashtable_get (client->http_headers,
                                                                   "origin"));
                                    }
                                    break;
                            }
                            relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                        }

                        ptr_real_ip = weechat_hashtable_get (
                            client->http_headers, "x-real-ip");
                        if (ptr_real_ip)
                        {
                            if (client->real_ip)
                                free (client->real_ip);
                            client->real_ip = strdup (ptr_real_ip);
                            relay_client_set_desc (client);
                            weechat_printf_date_tags (
                                NULL, 0, "relay_client",
                                _("%s: websocket client %s%s%s has real IP "
                                  "address \"%s\""),
                                RELAY_PLUGIN_NAME,
                                RELAY_COLOR_CHAT_CLIENT,
                                client->desc,
                                RELAY_COLOR_CHAT,
                                ptr_real_ip);
                        }

                        /* remove HTTP headers */
                        weechat_hashtable_free (client->http_headers);
                        client->http_headers = NULL;

                        /*
                         * discard all received data after the handshake
                         * received from client, and return immediately
                         */
                        free (client->partial_message);
                        client->partial_message = NULL;
                        weechat_string_free_split (lines);
                        return;
                    }
                }
                else
                {
                    /*
                     * interpret text from client, according to the relay
                     * protocol used
                     */
                    switch (client->protocol)
                    {
                        case RELAY_PROTOCOL_WEECHAT:
                            relay_weechat_recv (client, lines[i]);
                            break;
                        case RELAY_PROTOCOL_IRC:
                            relay_irc_recv (client, lines[i]);
                            break;
                        case RELAY_NUM_PROTOCOLS:
                            break;
                    }
                }
            }
            weechat_string_free_split (lines);
        }
        if (pos[1])
        {
            tmp = strdup (pos + 1);
            free (client->partial_message);
            client->partial_message = tmp;
        }
        else
        {
            free (client->partial_message);
            client->partial_message = NULL;
        }
    }
}

/*
 * Reads text buffer from a client.
 */

void
relay_client_recv_text_buffer (struct t_relay_client *client,
                               const char *buffer,
                               unsigned long long length_buffer)
{
    unsigned long long index;
    unsigned char msg_type;

    index = 0;
    while (index < length_buffer)
    {
        msg_type = RELAY_CLIENT_MSG_STANDARD;

        /*
         * in case of websocket, we can receive PING from client:
         * trace this PING in raw buffer and answer with a PONG
         */
        if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
        {
            msg_type = (unsigned char)buffer[index];
            if (msg_type == RELAY_CLIENT_MSG_PING)
            {
                /* print message in raw buffer */
                relay_raw_print (client, RELAY_CLIENT_MSG_PING,
                                 RELAY_RAW_FLAG_RECV | RELAY_RAW_FLAG_BINARY,
                                 buffer + index + 1,
                                 strlen (buffer + index + 1));
                /* answer with a PONG */
                relay_client_send (client,
                                   RELAY_CLIENT_MSG_PONG,
                                   buffer + index + 1,
                                   strlen (buffer + index + 1),
                                   NULL);
            }
            else if (msg_type == RELAY_CLIENT_MSG_CLOSE)
            {
                /* print message in raw buffer */
                relay_raw_print (client, RELAY_CLIENT_MSG_CLOSE,
                                 RELAY_RAW_FLAG_RECV | RELAY_RAW_FLAG_BINARY,
                                 buffer + index + 1,
                                 strlen (buffer + index + 1));
                /* answer with a CLOSE */
                relay_client_send (client,
                                   RELAY_CLIENT_MSG_CLOSE,
                                   buffer + index + 1,
                                   strlen (buffer + index + 1),
                                   NULL);
                /* close the connection */
                relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                /* ignore any other message after the close */
                return;
            }
            index++;
        }

        if (msg_type == RELAY_CLIENT_MSG_STANDARD)
            relay_client_recv_text (client, buffer + index);

        index += strlen (buffer + index) + 1;
    }
}

/*
 * Reads data from a client.
 */

int
relay_client_recv_cb (const void *pointer, void *data, int fd)
{
    struct t_relay_client *client;
    static char buffer[4096], decoded[8192 + 1];
    const char *ptr_buffer;
    int num_read, rc;
    unsigned long long decoded_length, length_buffer;

    /* make C compiler happy */
    (void) data;
    (void) fd;

    client = (struct t_relay_client *)pointer;

    if (client->sock < 0)
        return WEECHAT_RC_OK;

    /*
     * data can be received only during authentication
     * or if connected (authentication was OK)
     */
    if ((client->status != RELAY_STATUS_WAITING_AUTH)
        && (client->status != RELAY_STATUS_CONNECTED))
    {
        return WEECHAT_RC_OK;
    }

    if (client->tls)
        num_read = gnutls_record_recv (client->gnutls_sess, buffer,
                                       sizeof (buffer) - 1);
    else
        num_read = recv (client->sock, buffer, sizeof (buffer) - 1, 0);

    if (num_read > 0)
    {
        buffer[num_read] = '\0';
        ptr_buffer = buffer;
        length_buffer = num_read;

        /*
         * if we are receiving the first message from client, check if it looks
         * like a websocket
         */
        if (client->bytes_recv == 0)
        {
            if (relay_websocket_is_http_get_weechat (buffer))
            {
                /*
                 * web socket is just initializing for now, it's not accepted
                 * (we will check later with "http_headers" if web socket is
                 * valid or not)
                 */
                client->websocket = RELAY_CLIENT_WEBSOCKET_INITIALIZING;
                client->http_headers = weechat_hashtable_new (
                    32,
                    WEECHAT_HASHTABLE_STRING,
                    WEECHAT_HASHTABLE_STRING,
                    NULL, NULL);
            }
        }

        client->bytes_recv += num_read;

        if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
        {
            /* websocket used, decode message */
            rc = relay_websocket_decode_frame ((unsigned char *)buffer,
                                               (unsigned long long)num_read,
                                               (unsigned char *)decoded,
                                               &decoded_length);
            if (decoded_length == 0)
            {
                /*
                 * When decoded length is 0, assume client sent a PONG frame.
                 *
                 * RFC 6455 Section 5.5.3:
                 *
                 *   "A Pong frame MAY be sent unsolicited.  This serves as a
                 *   unidirectional heartbeat.  A response to an unsolicited
                 *   Pong frame is not expected."
                 */
                return WEECHAT_RC_OK;
            }
            if (!rc)
            {
                /* error when decoding frame: close connection */
                weechat_printf_date_tags (
                    NULL, 0, "relay_client",
                    _("%s%s: error decoding websocket frame for client "
                      "%s%s%s"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                    RELAY_COLOR_CHAT_CLIENT,
                    client->desc,
                    RELAY_COLOR_CHAT);
                relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                return WEECHAT_RC_OK;
            }
            ptr_buffer = decoded;
            length_buffer = decoded_length;
        }

        if ((client->websocket == RELAY_CLIENT_WEBSOCKET_INITIALIZING)
            || (client->recv_data_type == RELAY_CLIENT_DATA_TEXT))
        {
            /* websocket initializing or text data for this client */
            relay_client_recv_text_buffer (client, ptr_buffer, length_buffer);
        }
        else
        {
            /* receive buffer as-is (binary data) */
            /* currently, all supported protocols receive only text, no binary */
        }
        relay_buffer_refresh (NULL);
    }
    else
    {
        if (client->tls)
        {
            if ((num_read == 0)
                || ((num_read != GNUTLS_E_AGAIN) && (num_read != GNUTLS_E_INTERRUPTED)))
            {
                weechat_printf_date_tags (
                    NULL, 0, "relay_client",
                    _("%s%s: reading data on socket for client %s%s%s: "
                      "error %d %s"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                    RELAY_COLOR_CHAT_CLIENT,
                    client->desc,
                    RELAY_COLOR_CHAT,
                    num_read,
                    (num_read == 0) ? _("(connection closed by peer)") :
                    gnutls_strerror (num_read));
                relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
            }
        }
        else
        {
            if ((num_read == 0)
                || ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
            {
                weechat_printf_date_tags (
                    NULL, 0, "relay_client",
                    _("%s%s: reading data on socket for client %s%s%s: "
                      "error %d %s"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                    RELAY_COLOR_CHAT_CLIENT,
                    client->desc,
                    RELAY_COLOR_CHAT,
                    errno,
                    (num_read == 0) ? _("(connection closed by peer)") :
                    strerror (errno));
                relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Frees a message in out queue.
 */

void
relay_client_outqueue_free (struct t_relay_client *client,
                            struct t_relay_client_outqueue *outqueue)
{
    struct t_relay_client_outqueue *new_outqueue;

    if (!client || !outqueue)
        return;

    /* remove outqueue message */
    if (client->last_outqueue == outqueue)
        client->last_outqueue = outqueue->prev_outqueue;
    if (outqueue->prev_outqueue)
    {
        (outqueue->prev_outqueue)->next_outqueue = outqueue->next_outqueue;
        new_outqueue = client->outqueue;
    }
    else
        new_outqueue = outqueue->next_outqueue;

    if (outqueue->next_outqueue)
        (outqueue->next_outqueue)->prev_outqueue = outqueue->prev_outqueue;

    /* free data */
    if (outqueue->data)
        free (outqueue->data);
    if (outqueue->raw_message[0])
        free (outqueue->raw_message[0]);
    if (outqueue->raw_message[1])
        free (outqueue->raw_message[1]);
    free (outqueue);

    /* set new head */
    client->outqueue = new_outqueue;
}

/*
 * Frees all messages in out queue.
 */

void
relay_client_outqueue_free_all (struct t_relay_client *client)
{
    while (client->outqueue)
    {
        relay_client_outqueue_free (client, client->outqueue);
    }
}

/*
 * Sends messages in outqueue for a client.
 */

void
relay_client_send_outqueue (struct t_relay_client *client)
{
    int i, num_sent;
    char *buf;

    while (client->outqueue)
    {
        if (client->tls)
        {
            num_sent = (client->sock >= 0) ?
                gnutls_record_send (client->gnutls_sess,
                                    client->outqueue->data,
                                    client->outqueue->data_size) :
                client->outqueue->data_size;
        }
        else
        {
            num_sent = (client->sock >= 0) ?
                send (client->sock,
                      client->outqueue->data,
                      client->outqueue->data_size, 0) :
                client->outqueue->data_size;
        }
        if (num_sent >= 0)
        {
            for (i = 0; i < 2; i++)
            {
                if (client->outqueue->raw_message[i])
                {
                    /*
                     * print raw message and remove it from outqueue
                     * (so that it is displayed only one time, even if
                     * message is sent in many chunks)
                     */
                    relay_raw_print (
                        client,
                        client->outqueue->raw_msg_type[i],
                        client->outqueue->raw_flags[i],
                        client->outqueue->raw_message[i],
                        client->outqueue->raw_size[i]);
                    client->outqueue->raw_flags[i] = 0;
                    free (client->outqueue->raw_message[i]);
                    client->outqueue->raw_message[i] = NULL;
                    client->outqueue->raw_size[i] = 0;
                }
            }
            if (num_sent > 0)
            {
                client->bytes_sent += num_sent;
                relay_buffer_refresh (NULL);
            }
            if (num_sent == client->outqueue->data_size)
            {
                /* whole data sent, remove outqueue */
                relay_client_outqueue_free (client, client->outqueue);
            }
            else
            {
                /*
                 * some data was not sent, update outqueue and stop
                 * sending data from outqueue
                 */
                if (num_sent > 0)
                {
                    buf = malloc (client->outqueue->data_size - num_sent);
                    if (buf)
                    {
                        memcpy (buf,
                                client->outqueue->data + num_sent,
                                client->outqueue->data_size - num_sent);
                        free (client->outqueue->data);
                        client->outqueue->data = buf;
                        client->outqueue->data_size = client->outqueue->data_size - num_sent;
                    }
                }
                break;
            }
        }
        else
        {
            if (client->tls)
            {
                if ((num_sent == GNUTLS_E_AGAIN)
                    || (num_sent == GNUTLS_E_INTERRUPTED))
                {
                    /* we will retry later this client's queue */
                    break;
                }
                else
                {
                    weechat_printf_date_tags (
                        NULL, 0, "relay_client",
                        _("%s%s: sending data to client %s%s%s: "
                          "error %d %s"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        num_sent,
                        gnutls_strerror (num_sent));
                    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                }
            }
            else
            {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                {
                    /* we will retry later this client's queue */
                    break;
                }
                else
                {
                    weechat_printf_date_tags (
                        NULL, 0, "relay_client",
                        _("%s%s: sending data to client %s%s%s: "
                          "error %d %s"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        errno,
                        strerror (errno));
                    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                }
            }
        }
    }

    if (!client->outqueue && client->hook_timer_send)
    {
        weechat_unhook (client->hook_timer_send);
        client->hook_timer_send = NULL;
    }
}

/*
 * Timer called for a client when outqueue is not empty.
 *
 * This timer is automatically removed when the outqueue becomes empty
 * (done in the function relay_client_send_outqueue).
 */

int
relay_client_timer_send_cb (const void *pointer, void *data,
                            int remaining_calls)
{
    struct t_relay_client *ptr_client;

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    ptr_client = (struct t_relay_client *)pointer;

    relay_client_send_outqueue (ptr_client);

    return WEECHAT_RC_OK;
}

/*
 * Adds a message in out queue.
 */

void
relay_client_outqueue_add (struct t_relay_client *client,
                           const char *data, int data_size,
                           enum t_relay_client_msg_type raw_msg_type[2],
                           int raw_flags[2],
                           const char *raw_message[2],
                           int raw_size[2])
{
    struct t_relay_client_outqueue *new_outqueue;
    int i;

    if (!client || !data || (data_size <= 0))
        return;

    new_outqueue = malloc (sizeof (*new_outqueue));
    if (!new_outqueue)
        return;

    new_outqueue->data = malloc (data_size);
    if (!new_outqueue->data)
    {
        free (new_outqueue);
        return;
    }

    memcpy (new_outqueue->data, data, data_size);
    new_outqueue->data_size = data_size;
    for (i = 0; i < 2; i++)
    {
        new_outqueue->raw_msg_type[i] = RELAY_CLIENT_MSG_STANDARD;
        new_outqueue->raw_flags[i] = 0;
        new_outqueue->raw_message[i] = NULL;
        new_outqueue->raw_size[i] = 0;
        if (raw_message && raw_message[i] && (raw_size[i] > 0))
        {
            new_outqueue->raw_message[i] = malloc (raw_size[i]);
            if (new_outqueue->raw_message[i])
            {
                new_outqueue->raw_msg_type[i] = raw_msg_type[i];
                new_outqueue->raw_flags[i] = raw_flags[i];
                memcpy (new_outqueue->raw_message[i], raw_message[i],
                        raw_size[i]);
                new_outqueue->raw_size[i] = raw_size[i];
            }
        }
    }

    new_outqueue->prev_outqueue = client->last_outqueue;
    new_outqueue->next_outqueue = NULL;
    if (client->last_outqueue)
        client->last_outqueue->next_outqueue = new_outqueue;
    else
        client->outqueue = new_outqueue;
    client->last_outqueue = new_outqueue;

    if (!client->hook_timer_send)
    {
        client->hook_timer_send = weechat_hook_timer (
            1, 0, 0,
            &relay_client_timer_send_cb, client, NULL);
    }
}

/*
 * Sends data to client (adds in out queue if it's impossible to send now).
 *
 * If "message_raw_buffer" is not NULL, it is used for display in raw buffer
 * and replaces display of data, which is default.
 *
 * Returns number of bytes sent to client, -1 if error.
 */

int
relay_client_send (struct t_relay_client *client,
                   enum t_relay_client_msg_type msg_type,
                   const char *data,
                   int data_size, const char *message_raw_buffer)
{
    int num_sent, raw_size[2], raw_flags[2], opcode, i;
    enum t_relay_client_msg_type raw_msg_type[2];
    char *websocket_frame;
    unsigned long long length_frame;
    const char *ptr_data, *raw_msg[2];

    if (client->sock < 0)
        return -1;

    ptr_data = data;
    websocket_frame = NULL;

    /* set raw messages */
    for (i = 0; i < 2; i++)
    {
        raw_msg_type[i] = msg_type;
        raw_flags[i] = RELAY_RAW_FLAG_SEND;
        raw_msg[i] = NULL;
        raw_size[i] = 0;
    }
    if (message_raw_buffer)
    {
        if (weechat_relay_plugin->debug >= 2)
        {
            raw_msg[0] = message_raw_buffer;
            raw_size[0] = strlen (message_raw_buffer) + 1;
            raw_msg[1] = data;
            raw_size[1] = data_size;
            raw_flags[1] |= RELAY_RAW_FLAG_BINARY;
            if ((client->websocket == RELAY_CLIENT_WEBSOCKET_INITIALIZING)
                || (client->send_data_type == RELAY_CLIENT_DATA_TEXT))
            {
                raw_size[1]--;
            }
        }
        else
        {
            raw_msg[0] = message_raw_buffer;
            raw_size[0] = strlen (message_raw_buffer) + 1;
        }
    }
    else
    {
        raw_msg[0] = data;
        raw_size[0] = data_size;
        if ((msg_type == RELAY_CLIENT_MSG_PING)
            || (msg_type == RELAY_CLIENT_MSG_PONG)
            || (msg_type == RELAY_CLIENT_MSG_CLOSE)
            || ((client->websocket != RELAY_CLIENT_WEBSOCKET_INITIALIZING)
                && (client->send_data_type == RELAY_CLIENT_DATA_BINARY)))
        {
            /*
             * set binary flag if we send binary to client
             * (except if websocket is initializing and then we are sending
             * HTTP data, as text)
             */
            raw_flags[0] |= RELAY_RAW_FLAG_BINARY;
        }
        else
        {
            /* count the final '\0' in size */
            raw_size[0]++;
        }
    }

    /* if websocket is initialized, encode data in a websocket frame */
    if (client->websocket == RELAY_CLIENT_WEBSOCKET_READY)
    {
        switch (msg_type)
        {
            case RELAY_CLIENT_MSG_PING:
                opcode = WEBSOCKET_FRAME_OPCODE_PING;
                break;
            case RELAY_CLIENT_MSG_PONG:
                opcode = WEBSOCKET_FRAME_OPCODE_PONG;
                break;
            case RELAY_CLIENT_MSG_CLOSE:
                opcode = WEBSOCKET_FRAME_OPCODE_CLOSE;
                break;
            default:
                opcode = (client->send_data_type == RELAY_CLIENT_DATA_TEXT) ?
                    WEBSOCKET_FRAME_OPCODE_TEXT : WEBSOCKET_FRAME_OPCODE_BINARY;
                break;
        }
        websocket_frame = relay_websocket_encode_frame (opcode, data,
                                                        data_size,
                                                        &length_frame);
        if (websocket_frame)
        {
            ptr_data = websocket_frame;
            data_size = length_frame;
        }
    }

    num_sent = -1;

    /*
     * if outqueue is not empty, add to outqueue
     * (because message must be sent *after* messages already in outqueue)
     */
    if (client->outqueue)
    {
        relay_client_outqueue_add (client, ptr_data, data_size,
                                   raw_msg_type, raw_flags, raw_msg, raw_size);
    }
    else
    {
        if (client->tls)
        {
            num_sent = (client->sock >= 0) ?
                gnutls_record_send (client->gnutls_sess, ptr_data, data_size) :
                data_size;
        }
        else
        {
            num_sent = (client->sock >= 0) ?
                send (client->sock, ptr_data, data_size, 0) : data_size;
        }

        if (num_sent >= 0)
        {
            for (i = 0; i < 2; i++)
            {
                if (raw_msg[i])
                {
                    relay_raw_print (client, raw_msg_type[i], raw_flags[i],
                                     raw_msg[i], raw_size[i]);
                }
            }
            if (num_sent > 0)
            {
                client->bytes_sent += num_sent;
                relay_buffer_refresh (NULL);
            }
            if (num_sent < data_size)
            {
                /* some data was not sent, add it to outqueue */
                relay_client_outqueue_add (client,
                                           ptr_data + num_sent,
                                           data_size - num_sent,
                                           NULL, NULL, NULL, NULL);
            }
        }
        else
        {
            if (client->tls)
            {
                if ((num_sent == GNUTLS_E_AGAIN)
                    || (num_sent == GNUTLS_E_INTERRUPTED))
                {
                    /* add message to queue (will be sent later) */
                    relay_client_outqueue_add (client,
                                               ptr_data, data_size,
                                               raw_msg_type, raw_flags,
                                               raw_msg, raw_size);
                }
                else
                {
                    weechat_printf_date_tags (
                        NULL, 0, "relay_client",
                        _("%s%s: sending data to client %s%s%s: error %d %s"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        num_sent,
                        gnutls_strerror (num_sent));
                    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                }
            }
            else
            {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                {
                    /* add message to queue (will be sent later) */
                    relay_client_outqueue_add (client, ptr_data, data_size,
                                               raw_msg_type, raw_flags,
                                               raw_msg, raw_size);
                }
                else
                {
                    weechat_printf_date_tags (
                        NULL, 0, "relay_client",
                        _("%s%s: sending data to client %s%s%s: error %d %s"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME,
                        RELAY_COLOR_CHAT_CLIENT,
                        client->desc,
                        RELAY_COLOR_CHAT,
                        errno,
                        strerror (errno));
                    relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
                }
            }
        }
    }

    if (websocket_frame)
        free (websocket_frame);

    return num_sent;
}

/*
 * Timer callback, called each second.
 */

int
relay_client_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    struct t_relay_client *ptr_client, *ptr_next_client;
    int purge_delay, auth_timeout;
    time_t current_time;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    purge_delay = weechat_config_integer (relay_config_network_clients_purge_delay);
    auth_timeout = weechat_config_integer (relay_config_network_auth_timeout);

    current_time = time (NULL);

    ptr_client = relay_clients;
    while (ptr_client)
    {
        ptr_next_client = ptr_client->next_client;

        if (RELAY_CLIENT_HAS_ENDED(ptr_client))
        {
            if ((purge_delay >= 0)
                && (current_time >= ptr_client->end_time + (purge_delay * 60)))
            {
                relay_client_free (ptr_client);
                relay_buffer_refresh (NULL);
            }
        }
        else if (ptr_client->sock >= 0)
        {
            /* send messages in outqueue */
            relay_client_send_outqueue (ptr_client);

            /* disconnect clients not authenticated */
            if ((auth_timeout > 0)
                && (ptr_client->status == RELAY_STATUS_WAITING_AUTH))
            {
                if (current_time - ptr_client->start_time > auth_timeout)
                {
                    relay_client_set_status (ptr_client,
                                             RELAY_STATUS_AUTH_FAILED);
                }
            }
        }

        ptr_client = ptr_next_client;
    }

    return WEECHAT_RC_OK;
}

/*
 * Creates a new client.
 *
 * Returns pointer to new client, NULL if error.
 */

struct t_relay_client *
relay_client_new (int sock, const char *address, struct t_relay_server *server)
{
    struct t_relay_client *new_client;
    int plain_text_password;
    int bits;
    struct t_config_option *ptr_option;

    new_client = malloc (sizeof (*new_client));
    if (new_client)
    {
        new_client->id = (relay_clients) ? relay_clients->id + 1 : 1;
        new_client->desc = NULL;
        new_client->sock = sock;
        new_client->server_port = server->port;
        new_client->tls = server->tls;
        new_client->gnutls_sess = NULL;
        new_client->hook_timer_handshake = NULL;
        new_client->gnutls_handshake_ok = 0;
        new_client->websocket = RELAY_CLIENT_WEBSOCKET_NOT_USED;
        new_client->http_headers = NULL;
        new_client->address = strdup ((address && address[0]) ?
                                      address : "local");
        new_client->real_ip = NULL;
        new_client->status = RELAY_STATUS_CONNECTING;
        new_client->protocol = server->protocol;
        new_client->protocol_string = (server->protocol_string) ? strdup (server->protocol_string) : NULL;
        new_client->protocol_args = (server->protocol_args) ? strdup (server->protocol_args) : NULL;
        new_client->nonce = relay_auth_generate_nonce (
            weechat_config_integer (relay_config_network_nonce_size));
        plain_text_password = weechat_string_match_list (
            relay_auth_password_hash_algo_name[0],
            (const char **)relay_config_network_password_hash_algo_list,
            1);
        new_client->password_hash_algo = (plain_text_password) ? 0 : -1;
        new_client->password_hash_iterations = weechat_config_integer (
            relay_config_network_password_hash_iterations);
        new_client->listen_start_time = server->start_time;
        new_client->start_time = time (NULL);
        new_client->end_time = 0;
        new_client->hook_fd = NULL;
        new_client->hook_timer_send = NULL;
        new_client->last_activity = new_client->start_time;
        new_client->bytes_recv = 0;
        new_client->bytes_sent = 0;
        switch (new_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                new_client->recv_data_type = RELAY_CLIENT_DATA_TEXT;
                new_client->send_data_type = RELAY_CLIENT_DATA_BINARY;
                break;
            case RELAY_PROTOCOL_IRC:
                new_client->recv_data_type = RELAY_CLIENT_DATA_TEXT;
                new_client->send_data_type = RELAY_CLIENT_DATA_TEXT;
                break;
            default:
                new_client->recv_data_type = RELAY_CLIENT_DATA_TEXT;
                new_client->send_data_type = RELAY_CLIENT_DATA_TEXT;
                break;
        }
        new_client->partial_message = NULL;

        relay_client_set_desc (new_client);

        if (new_client->tls)
        {
            if (!relay_network_init_tls_cert_key_ok)
            {
                weechat_printf_date_tags (
                    NULL, 0, "relay_client",
                    _("%s%s: warning: no TLS certificate/key found (option "
                      "relay.network.tls_cert_key)"),
                    weechat_prefix ("error"),
                    RELAY_PLUGIN_NAME);
            }
            new_client->status = RELAY_STATUS_CONNECTING;
            /*
             * set Diffie-Hellman parameters on first TLS connection from a
             * client (done only one time)
             */
            if (!relay_gnutls_dh_params)
            {
                relay_gnutls_dh_params = malloc (sizeof (*relay_gnutls_dh_params));
                if (relay_gnutls_dh_params)
                {
                    gnutls_dh_params_init (relay_gnutls_dh_params);
#if LIBGNUTLS_VERSION_NUMBER >= 0x020c00 /* 2.12.0 */
                    bits = gnutls_sec_param_to_pk_bits (GNUTLS_PK_DH,
                                                        GNUTLS_SEC_PARAM_LOW);
#else
                    /* default for old gnutls */
                    bits = 1024;
#endif /* LIBGNUTLS_VERSION_NUMBER >= 0x020c00 */
                    gnutls_dh_params_generate2 (*relay_gnutls_dh_params, bits);
                    gnutls_certificate_set_dh_params (relay_gnutls_x509_cred,
                                                      *relay_gnutls_dh_params);
                }
            }
            gnutls_init (&(new_client->gnutls_sess), GNUTLS_SERVER);
            if (relay_gnutls_priority_cache)
                gnutls_priority_set (new_client->gnutls_sess, *relay_gnutls_priority_cache);
            gnutls_credentials_set (new_client->gnutls_sess, GNUTLS_CRD_CERTIFICATE, relay_gnutls_x509_cred);
            gnutls_certificate_server_set_request (new_client->gnutls_sess, GNUTLS_CERT_IGNORE);
            gnutls_transport_set_ptr (new_client->gnutls_sess,
                                      (gnutls_transport_ptr_t) ((ptrdiff_t) new_client->sock));
            ptr_option = weechat_config_get ("weechat.network.gnutls_handshake_timeout");
            new_client->hook_timer_handshake = weechat_hook_timer (1000 / 10,
                                                                   0,
                                                                   (ptr_option) ?
                                                                   weechat_config_integer (ptr_option) * 10 : 30 * 10,
                                                                   &relay_client_handshake_timer_cb,
                                                                   new_client,
                                                                   NULL);
        }

        new_client->protocol_data = NULL;
        switch (new_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_alloc (new_client);
                if (!new_client->tls)
                {
                    new_client->status =
                        relay_weechat_get_initial_status (new_client);
                }
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_alloc (new_client);
                if (!new_client->tls)
                {
                    new_client->status =
                        relay_irc_get_initial_status (new_client);
                }
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }

        new_client->outqueue = NULL;
        new_client->last_outqueue = NULL;

        new_client->prev_client = NULL;
        new_client->next_client = relay_clients;
        if (relay_clients)
            relay_clients->prev_client = new_client;
        else
            last_relay_client = new_client;
        relay_clients = new_client;

        if (server->unix_socket)
        {
            weechat_printf_date_tags (
                NULL, 0, "relay_client",
                _("%s: new client on path %s: %s%s%s (%s)"),
                RELAY_PLUGIN_NAME,
                server->path,
                RELAY_COLOR_CHAT_CLIENT,
                new_client->desc,
                RELAY_COLOR_CHAT,
                _(relay_client_status_string[new_client->status]));
        }
        else
        {
            weechat_printf_date_tags (
                NULL, 0, "relay_client",
                _("%s: new client on port %s: %s%s%s (%s)"),
                RELAY_PLUGIN_NAME,
                server->path,
                RELAY_COLOR_CHAT_CLIENT,
                new_client->desc,
                RELAY_COLOR_CHAT,
                _(relay_client_status_string[new_client->status]));
        }

        if (new_client->sock >= 0)
        {
            new_client->hook_fd = weechat_hook_fd (new_client->sock,
                                                   1, 0, 0,
                                                   &relay_client_recv_cb,
                                                   new_client, NULL);
        }

        relay_client_count++;

        if (!relay_buffer
            && weechat_config_boolean (relay_config_look_auto_open_buffer))
        {
            relay_buffer_open ();
        }

        relay_client_send_signal (new_client);

        relay_buffer_refresh (WEECHAT_HOTLIST_PRIVATE);
    }
    else
    {
        weechat_printf_date_tags (NULL, 0, "relay_client",
                                  _("%s%s: not enough memory for new client"),
                                  weechat_prefix ("error"), RELAY_PLUGIN_NAME);
    }

    return new_client;
}

/*
 * Creates a new client using an infolist.
 *
 * This is called to restore clients after /upgrade.
 */

struct t_relay_client *
relay_client_new_with_infolist (struct t_infolist *infolist)
{
    struct t_relay_client *new_client;
    const char *str;

    new_client = malloc (sizeof (*new_client));
    if (new_client)
    {
        new_client->id = weechat_infolist_integer (infolist, "id");
        new_client->desc = NULL;
        new_client->sock = weechat_infolist_integer (infolist, "sock");
        new_client->server_port = weechat_infolist_integer (infolist, "server_port");
        /* "tls" replaces "ssl" in WeeChat 4.0.0 */
        if (weechat_infolist_search_var (infolist, "tls"))
            new_client->tls = weechat_infolist_integer (infolist, "tls");
        else
            new_client->tls = weechat_infolist_integer (infolist, "ssl");
        new_client->gnutls_sess = NULL;
        new_client->hook_timer_handshake = NULL;
        new_client->gnutls_handshake_ok = 0;
        new_client->websocket = weechat_infolist_integer (infolist, "websocket");
        new_client->http_headers = NULL;
        new_client->address = strdup (weechat_infolist_string (infolist, "address"));
        str = weechat_infolist_string (infolist, "real_ip");
        new_client->real_ip = (str) ? strdup (str) : NULL;
        new_client->status = weechat_infolist_integer (infolist, "status");
        new_client->protocol = weechat_infolist_integer (infolist, "protocol");
        str = weechat_infolist_string (infolist, "protocol_string");
        new_client->protocol_string = (str) ? strdup (str) : NULL;
        str = weechat_infolist_string (infolist, "protocol_args");
        new_client->protocol_args = (str) ? strdup (str) : NULL;
        /* "nonce" is new in WeeChat 2.9 */
        if (weechat_infolist_search_var (infolist, "nonce"))
            new_client->nonce = strdup (weechat_infolist_string (infolist, "nonce"));
        else
            new_client->nonce = relay_auth_generate_nonce (
                weechat_config_integer (relay_config_network_nonce_size));
        /* "password_hash_algo" is new in WeeChat 2.9 */
        if (weechat_infolist_search_var (infolist, "password_hash_algo"))
            new_client->password_hash_algo = weechat_infolist_integer (infolist, "password_hash_algo");
        else
            new_client->password_hash_algo = RELAY_AUTH_PASSWORD_HASH_PLAIN;
        /* "password_hash_iterations" is new in WeeChat 2.9 */
        if (weechat_infolist_search_var (infolist, "password_hash_iterations"))
            new_client->password_hash_iterations = weechat_infolist_integer (infolist, "password_hash_iterations");
        else
            new_client->password_hash_iterations = weechat_config_integer (
                relay_config_network_password_hash_iterations);
        new_client->listen_start_time = weechat_infolist_time (infolist, "listen_start_time");
        new_client->start_time = weechat_infolist_time (infolist, "start_time");
        new_client->end_time = weechat_infolist_time (infolist, "end_time");
        if (new_client->sock >= 0)
        {
            new_client->hook_fd = weechat_hook_fd (new_client->sock,
                                                   1, 0, 0,
                                                   &relay_client_recv_cb,
                                                   new_client,
                                                   NULL);
        }
        else
            new_client->hook_fd = NULL;
        new_client->hook_timer_send = NULL;
        new_client->last_activity = weechat_infolist_time (infolist, "last_activity");
        sscanf (weechat_infolist_string (infolist, "bytes_recv"),
                "%llu", &(new_client->bytes_recv));
        sscanf (weechat_infolist_string (infolist, "bytes_sent"),
                "%llu", &(new_client->bytes_sent));
        new_client->recv_data_type = weechat_infolist_integer (infolist, "recv_data_type");
        new_client->send_data_type = weechat_infolist_integer (infolist, "send_data_type");
        str = weechat_infolist_string (infolist, "partial_message");
        new_client->partial_message = (str) ? strdup (str) : NULL;

        str = weechat_infolist_string (infolist, "desc");
        if (str)
            new_client->desc = strdup (str);
        else
            relay_client_set_desc (new_client);

        switch (new_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_alloc_with_infolist (new_client,
                                                   infolist);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_alloc_with_infolist (new_client,
                                               infolist);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }

        new_client->outqueue = NULL;
        new_client->last_outqueue = NULL;

        new_client->prev_client = NULL;
        new_client->next_client = relay_clients;
        if (relay_clients)
            relay_clients->prev_client = new_client;
        else
            last_relay_client = new_client;
        relay_clients = new_client;

        relay_client_count++;
    }

    return new_client;
}

/*
 * Sets status for a client.
 */

void
relay_client_set_status (struct t_relay_client *client,
                         enum t_relay_status status)
{
    struct t_relay_server *ptr_server;

    /*
     * IMPORTANT: if changes are made in this function or sub-functions called,
     * please also update the function relay_client_add_to_infolist:
     * when the flag force_disconnected_state is set to 1 we simulate
     * a disconnected state for client in infolist (used on /upgrade -save)
     */

    client->status = status;

    if (client->status == RELAY_STATUS_CONNECTED)
    {
        weechat_printf_date_tags (
                    NULL, 0, "relay_client",
                    _("%s: client %s%s%s connected/authenticated"),
                    RELAY_PLUGIN_NAME,
                    RELAY_COLOR_CHAT_CLIENT,
                    client->desc,
                    RELAY_COLOR_CHAT);
    }
    else if (RELAY_CLIENT_HAS_ENDED(client))
    {
        client->end_time = time (NULL);

        ptr_server = relay_server_search (client->protocol_string);
        if (ptr_server)
            ptr_server->last_client_disconnect = client->end_time;

        relay_client_outqueue_free_all (client);

        if (client->hook_timer_handshake)
        {
            weechat_unhook (client->hook_timer_handshake);
            client->hook_timer_handshake = NULL;
        }
        client->gnutls_handshake_ok = 0;
        if (client->hook_fd)
        {
            weechat_unhook (client->hook_fd);
            client->hook_fd = NULL;
        }
        if (client->hook_timer_send)
        {
            weechat_unhook (client->hook_timer_send);
            client->hook_timer_send = NULL;
        }
        switch (client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_close_connection (client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_close_connection (client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
        switch (client->status)
        {
            case RELAY_STATUS_AUTH_FAILED:
                weechat_printf_date_tags (NULL, 0, "relay_client",
                                          _("%s%s: authentication failed with "
                                            "client %s%s%s"),
                                          weechat_prefix ("error"),
                                          RELAY_PLUGIN_NAME,
                                          RELAY_COLOR_CHAT_CLIENT,
                                          client->desc,
                                          RELAY_COLOR_CHAT);
                break;
            case RELAY_STATUS_DISCONNECTED:
                weechat_printf_date_tags (
                    NULL, 0, "relay_client",
                    _("%s: disconnected from client %s%s%s"),
                    RELAY_PLUGIN_NAME,
                    RELAY_COLOR_CHAT_CLIENT,
                    client->desc,
                    RELAY_COLOR_CHAT);
                break;
            default:
                break;
        }

        if (client->sock >= 0)
        {
            if (client->tls && client->gnutls_handshake_ok)
                gnutls_bye (client->gnutls_sess, GNUTLS_SHUT_WR);
            close (client->sock);
            client->sock = -1;
            if (client->tls)
                gnutls_deinit (client->gnutls_sess);
        }
    }

    relay_client_send_signal (client);

    relay_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
}

/*
 * Removes a client.
 */

void
relay_client_free (struct t_relay_client *client)
{
    struct t_relay_client *new_relay_clients;

    if (!client)
        return;

    /* remove client from list */
    if (last_relay_client == client)
        last_relay_client = client->prev_client;
    if (client->prev_client)
    {
        (client->prev_client)->next_client = client->next_client;
        new_relay_clients = relay_clients;
    }
    else
        new_relay_clients = client->next_client;
    if (client->next_client)
        (client->next_client)->prev_client = client->prev_client;

    /* free data */
    if (client->desc)
        free (client->desc);
    if (client->address)
        free (client->address);
    if (client->real_ip)
        free (client->real_ip);
    if (client->protocol_string)
        free (client->protocol_string);
    if (client->protocol_args)
        free (client->protocol_args);
    if (client->nonce)
        free (client->nonce);
    if (client->hook_timer_handshake)
        weechat_unhook (client->hook_timer_handshake);
    if (client->http_headers)
        weechat_hashtable_free (client->http_headers);
    if (client->hook_fd)
        weechat_unhook (client->hook_fd);
    if (client->hook_timer_send)
        weechat_unhook (client->hook_timer_send);
    if (client->partial_message)
        free (client->partial_message);
    if (client->protocol_data)
    {
        switch (client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_free (client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_free (client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
    }
    relay_client_outqueue_free_all (client);

    free (client);

    relay_clients = new_relay_clients;

    relay_client_count--;
    if (relay_buffer_selected_line >= relay_client_count)
    {
        relay_buffer_selected_line = (relay_client_count == 0) ?
            0 : relay_client_count - 1;
    }
}

/*
 * Removes all clients.
 */

void
relay_client_free_all ()
{
    while (relay_clients)
    {
        relay_client_free (relay_clients);
    }
}

/*
 * Disconnects one client.
 */

void
relay_client_disconnect (struct t_relay_client *client)
{
    if (client->sock >= 0)
    {
        relay_client_set_status (client, RELAY_STATUS_DISCONNECTED);
    }
}

/*
 * Disconnects all clients.
 */

void
relay_client_disconnect_all ()
{
    struct t_relay_client *ptr_client;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        relay_client_disconnect (ptr_client);
    }
}

/*
 * Adds a client in an infolist.
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
relay_client_add_to_infolist (struct t_infolist *infolist,
                              struct t_relay_client *client,
                              int force_disconnected_state)
{
    struct t_infolist_item *ptr_item;
    char value[128];

    if (!infolist || !client)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_integer (ptr_item, "id", client->id))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "desc", client->desc))
        return 0;
    if (!RELAY_CLIENT_HAS_ENDED(client) && force_disconnected_state)
    {
        if (!weechat_infolist_new_var_integer (ptr_item, "sock", -1))
            return 0;
        if (!weechat_infolist_new_var_pointer (ptr_item, "hook_timer_handshake", NULL))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "gnutls_handshake_ok", 0))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "status", RELAY_STATUS_DISCONNECTED))
            return 0;
        if (!weechat_infolist_new_var_time (ptr_item, "end_time", time (NULL)))
            return 0;
        if (!weechat_infolist_new_var_string (ptr_item, "partial_message", NULL))
            return 0;
    }
    else
    {
        if (!weechat_infolist_new_var_integer (ptr_item, "sock", client->sock))
            return 0;
        if (!weechat_infolist_new_var_pointer (ptr_item, "hook_timer_handshake", client->hook_timer_handshake))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "gnutls_handshake_ok", client->gnutls_handshake_ok))
            return 0;
        if (!weechat_infolist_new_var_integer (ptr_item, "status", client->status))
            return 0;
        if (!weechat_infolist_new_var_time (ptr_item, "end_time", client->end_time))
            return 0;
        if (!weechat_infolist_new_var_string (ptr_item, "partial_message", client->partial_message))
            return 0;
    }
    if (!weechat_infolist_new_var_integer (ptr_item, "server_port", client->server_port))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "tls", client->tls))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "websocket", client->websocket))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "address", client->address))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "real_ip", client->real_ip))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "status_string", relay_client_status_string[client->status]))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "protocol", client->protocol))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_string", relay_protocol_string[client->protocol]))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_string", client->protocol_string))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "protocol_args", client->protocol_args))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "nonce", client->nonce))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "password_hash_algo", client->password_hash_algo))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "password_hash_iterations", client->password_hash_iterations))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "listen_start_time", client->listen_start_time))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "start_time", client->start_time))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_fd", client->hook_fd))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook_timer_send", client->hook_timer_send))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "last_activity", client->last_activity))
        return 0;
    snprintf (value, sizeof (value), "%llu", client->bytes_recv);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_recv", value))
        return 0;
    snprintf (value, sizeof (value), "%llu", client->bytes_sent);
    if (!weechat_infolist_new_var_string (ptr_item, "bytes_sent", value))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "recv_data_type", client->recv_data_type))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "send_data_type", client->send_data_type))
        return 0;

    switch (client->protocol)
    {
        case RELAY_PROTOCOL_WEECHAT:
            relay_weechat_add_to_infolist (ptr_item, client,
                                           force_disconnected_state);
            break;
        case RELAY_PROTOCOL_IRC:
            relay_irc_add_to_infolist (ptr_item, client,
                                       force_disconnected_state);
            break;
        case RELAY_NUM_PROTOCOLS:
            break;
    }

    return 1;
}

/*
 * Prints clients in WeeChat log file (usually for crash dump).
 */

void
relay_client_print_log ()
{
    struct t_relay_client *ptr_client;

    for (ptr_client = relay_clients; ptr_client;
         ptr_client = ptr_client->next_client)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[relay client (addr:0x%lx)]", ptr_client);
        weechat_log_printf ("  id. . . . . . . . . . . . : %d",   ptr_client->id);
        weechat_log_printf ("  desc. . . . . . . . . . . : '%s'", ptr_client->desc);
        weechat_log_printf ("  sock. . . . . . . . . . . : %d",   ptr_client->sock);
        weechat_log_printf ("  server_port . . . . . . . : %d",   ptr_client->server_port);
        weechat_log_printf ("  tls . . . . . . . . . . . : %d",   ptr_client->tls);
        weechat_log_printf ("  gnutls_sess . . . . . . . : 0x%lx", ptr_client->gnutls_sess);
        weechat_log_printf ("  hook_timer_handshake. . . : 0x%lx", ptr_client->hook_timer_handshake);
        weechat_log_printf ("  gnutls_handshake_ok . . . : 0x%lx", ptr_client->gnutls_handshake_ok);
        weechat_log_printf ("  websocket . . . . . . . . : %d",   ptr_client->websocket);
        weechat_log_printf ("  http_headers. . . . . . . : 0x%lx (hashtable: '%s')",
                            ptr_client->http_headers,
                            weechat_hashtable_get_string (ptr_client->http_headers, "keys_values"));
        weechat_log_printf ("  address . . . . . . . . . : '%s'", ptr_client->address);
        weechat_log_printf ("  real_ip . . . . . . . . . : '%s'", ptr_client->real_ip);
        weechat_log_printf ("  status. . . . . . . . . . : %d (%s)",
                            ptr_client->status,
                            relay_client_status_string[ptr_client->status]);
        weechat_log_printf ("  protocol. . . . . . . . . : %d (%s)",
                            ptr_client->protocol,
                            relay_protocol_string[ptr_client->protocol]);
        weechat_log_printf ("  protocol_string . . . . . : '%s'",  ptr_client->protocol_string);
        weechat_log_printf ("  protocol_args . . . . . . : '%s'",  ptr_client->protocol_args);
        weechat_log_printf ("  nonce . . . . . . . . . . : '%s'",  ptr_client->nonce);
        weechat_log_printf ("  password_hash_algo. . . . : %d (%s)",
                            ptr_client->password_hash_algo,
                            (ptr_client->password_hash_algo >= 0) ?
                            relay_auth_password_hash_algo_name[ptr_client->password_hash_algo] : "");
        weechat_log_printf ("  password_hash_iterations. : %d",    ptr_client->password_hash_iterations);
        weechat_log_printf ("  listen_start_time . . . . : %lld",  (long long)ptr_client->listen_start_time);
        weechat_log_printf ("  start_time. . . . . . . . : %lld",  (long long)ptr_client->start_time);
        weechat_log_printf ("  end_time. . . . . . . . . : %lld",  (long long)ptr_client->end_time);
        weechat_log_printf ("  hook_fd . . . . . . . . . : 0x%lx", ptr_client->hook_fd);
        weechat_log_printf ("  hook_timer_send . . . . . : 0x%lx", ptr_client->hook_timer_send);
        weechat_log_printf ("  last_activity . . . . . . : %lld",  (long long)ptr_client->last_activity);
        weechat_log_printf ("  bytes_recv. . . . . . . . : %llu",  ptr_client->bytes_recv);
        weechat_log_printf ("  bytes_sent. . . . . . . . : %llu",  ptr_client->bytes_sent);
        weechat_log_printf ("  recv_data_type. . . . . . : %d (%s)",
                            ptr_client->recv_data_type,
                            relay_client_data_type_string[ptr_client->recv_data_type]);
        weechat_log_printf ("  send_data_type. . . . . . : %d (%s)",
                            ptr_client->send_data_type,
                            relay_client_data_type_string[ptr_client->send_data_type]);
        weechat_log_printf ("  partial_message . . . . . : '%s'",  ptr_client->partial_message);
        weechat_log_printf ("  protocol_data . . . . . . : 0x%lx", ptr_client->protocol_data);
        switch (ptr_client->protocol)
        {
            case RELAY_PROTOCOL_WEECHAT:
                relay_weechat_print_log (ptr_client);
                break;
            case RELAY_PROTOCOL_IRC:
                relay_irc_print_log (ptr_client);
                break;
            case RELAY_NUM_PROTOCOLS:
                break;
        }
        weechat_log_printf ("  outqueue. . . . . . . . . : 0x%lx", ptr_client->outqueue);
        weechat_log_printf ("  last_outqueue . . . . . . : 0x%lx", ptr_client->last_outqueue);
        weechat_log_printf ("  prev_client . . . . . . . : 0x%lx", ptr_client->prev_client);
        weechat_log_printf ("  next_client . . . . . . . : 0x%lx", ptr_client->next_client);
    }
}
