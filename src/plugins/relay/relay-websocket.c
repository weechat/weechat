/*
 * relay-websocket.c - websocket server functions for relay plugin (RFC 6455)
 *
 * Copyright (C) 2013-2014 Sébastien Helleu <flashcode@flashtux.org>
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
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <gcrypt.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"
#include "relay-config.h"


/*
 * globally unique identifier that is concatenated to HTTP header
 * "Sec-WebSocket-Key"
 */
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


/*
 * Checks if a message is a HTTP GET with resource "/weechat".
 *
 * Returns:
 *   1: message is a HTTP GET with resource "/weechat"
 *   0: message is NOT a HTTP GET with resource "/weechat"
 */

int
relay_websocket_is_http_get_weechat (const char *message)
{
    /* the message must start with "GET /weechat" */
    if (strncmp (message, "GET /weechat", 12) != 0)
        return 0;

    /* after "GET /weechat", only a new line or " HTTP" is allowed */
    if ((message[12] != '\r') && (message[12] != '\n')
        && (strncmp (message + 12, " HTTP", 5) != 0))
    {
        return 0;
    }

    /* valid HTTP GET for resource "/weechat" */
    return 1;
}

/*
 * Saves a HTTP header in hashtable "http_header" of client.
 */

void
relay_websocket_save_header (struct t_relay_client *client,
                             const char *message)
{
    char *pos, *name;
    const char *ptr_value;

    /* ignore the "GET" request */
    if (strncmp (message, "GET ", 4) == 0)
        return;

    pos = strchr (message, ':');

    /* not a valid header */
    if (!pos || (pos == message))
        return;

    /* get header name */
    name = weechat_strndup (message, pos - message);
    if (!name)
        return;

    /* get pointer on header value */
    ptr_value = pos + 1;
    while (ptr_value[0] == ' ')
    {
        ptr_value++;
    }

    /* add header in the hashtable */
    weechat_hashtable_set (client->http_headers, name, ptr_value);

    free (name);
}

/*
 * Checks if a client handshake is valid.
 *
 * A websocket query looks like:
 *   GET /weechat HTTP/1.1
 *   Upgrade: websocket
 *   Connection: Upgrade
 *   Host: myhost:5000
 *   Origin: http://example.org.org
 *   Pragma: no-cache
 *   Cache-Control: no-cache
 *   Sec-WebSocket-Key: fo1J9uHSsrfDP3BkwUylzQ==
 *   Sec-WebSocket-Version: 13
 *   Sec-WebSocket-Extensions: x-webkit-deflate-frame
 *   Cookie: csrftoken=acb65377798f32dc377ebb50316a12b5
 *
 * Expected HTTP headers with values are:
 *
 *   header              | value
 *   --------------------+----------------
 *   "Upgrade"           | "websocket"
 *   "Sec-WebSocket-Key" | non-empty value
 *
 * If option relay.network.websocket_allowed_origins is set, the HTTP header
 * "Origin" is checked against this regex. If header "Origin" is not set or does
 * not match regex, the handshake is considered as invalid.
 *
 * Returns:
 *    0: handshake is valid
 *   -1: handshake is invalid (headers missing or with bad value)
 *   -2: origin is not allowed (option relay.network.websocket_allowed_origins)
 */

int
relay_websocket_client_handshake_valid (struct t_relay_client *client)
{
    const char *value;

    /* check if we have header "Upgrade" with value "websocket" */
    value = weechat_hashtable_get (client->http_headers, "Upgrade");
    if (!value)
        return -1;
    if (strcmp (value, "websocket") != 0 && (strcmp (value, "Websocket") != 0))
        return -1;

    /* check if we have header "Sec-WebSocket-Key" with non-empty value */
    value = weechat_hashtable_get (client->http_headers, "Sec-WebSocket-Key");
    if (!value || !value[0])
        return -1;

    if (relay_config_regex_websocket_allowed_origins)
    {
        value = weechat_hashtable_get (client->http_headers, "Origin");
        if (!value || !value[0])
            return -2;
        if (regexec (relay_config_regex_websocket_allowed_origins, value, 0,
                     NULL, 0) != 0)
        {
            return -2;
        }
    }

    /* client handshake is valid */
    return 0;
}

/*
 * Builds the handshake that will be returned to client, to initialize and use
 * the websocket.
 *
 * Returns a string with content of handshake to send to client, it looks like:
 *   HTTP/1.1 101 Switching Protocols
 *   Upgrade: websocket
 *   Connection: Upgrade
 *   Sec-WebSocket-Accept: 73OzoF/IyV9znm7Tsb4EtlEEmn4=
 *
 * Note: result must be freed after use.
 */

char *
relay_websocket_build_handshake (struct t_relay_client *client)
{
    const char *sec_websocket_key;
    char *key, sec_websocket_accept[128], handshake[1024];
    unsigned char *result;
    gcry_md_hd_t hd;
    int length;

    sec_websocket_key = weechat_hashtable_get (client->http_headers,
                                               "Sec-WebSocket-Key");
    if (!sec_websocket_key || !sec_websocket_key[0])
        return NULL;

    length = strlen (sec_websocket_key) + strlen (WEBSOCKET_GUID) + 1;
    key = malloc (length);
    if (!key)
        return NULL;

    /*
     * concatenate header "Sec-WebSocket-Key" with the GUID
     * (globally unique identifier)
     */
    snprintf (key, length, "%s%s", sec_websocket_key, WEBSOCKET_GUID);

    /* compute 160-bit SHA1 on the key and encode it with base64 */
    gcry_md_open (&hd, GCRY_MD_SHA1, 0);
    length = gcry_md_get_algo_dlen (GCRY_MD_SHA1);
    gcry_md_write (hd, key, strlen (key));
    result = gcry_md_read (hd, GCRY_MD_SHA1);
    weechat_string_encode_base64 ((char *)result, length, sec_websocket_accept);
    gcry_md_close (hd);

    /* build the handshake (it will be sent as-is to client) */
    snprintf (handshake, sizeof (handshake),
              "HTTP/1.1 101 Switching Protocols\r\n"
              "Upgrade: websocket\r\n"
              "Connection: Upgrade\r\n"
              //"Sec-WebSocket-Protocol: chat\r\n"
              "Sec-WebSocket-Accept: %s\r\n"
              "\r\n",
              sec_websocket_accept);

    return strdup (handshake);
}

/*
 * Sends a HTTP message to client.
 *
 * Argument "http" is a HTTP code + message, for example:
 * "403 Forbidden".
 */

void
relay_websocket_send_http (struct t_relay_client *client,
                           const char *http)
{
    char *message;
    int length;

    length = 32 + strlen (http) + 1;
    message = malloc (length);
    if (message)
    {
        snprintf (message, length, "HTTP/1.1 %s\r\n\r\n", http);
        relay_client_send (client, message, strlen (message), NULL);
        free (message);
    }
}

/*
 * Decodes a websocket frame.
 *
 * Returns:
 *   1: frame decoded successfully
 *   0: error decoding frame (connection must be closed if it happens)
 */

int
relay_websocket_decode_frame (const unsigned char *buffer,
                              unsigned long long buffer_length,
                              unsigned char *decoded,
                              unsigned long long *decoded_length)
{
    unsigned long long i, index_buffer, length_frame_size, length_frame;

    *decoded_length = 0;
    index_buffer = 0;

    /* loop to decode all frames in message */
    while (index_buffer + 2 <= buffer_length)
    {
        /*
         * check if frame is masked: client MUST send a masked frame; if frame is
         * not masked, we MUST reject it and close the connection (see RFC 6455)
         */
        if (!(buffer[index_buffer + 1] & 128))
            return 0;

        /* decode frame */
        length_frame_size = 1;
        length_frame = buffer[index_buffer + 1] & 127;
        index_buffer += 2;
        if ((length_frame == 126) || (length_frame == 127))
        {
            length_frame_size = (length_frame == 126) ? 2 : 8;
            if (buffer_length < 1 + length_frame_size)
                return 0;
            length_frame = 0;
            for (i = 0; i < length_frame_size; i++)
            {
                length_frame += (unsigned long long)buffer[index_buffer + i] << ((length_frame_size - i - 1) * 8);
            }
            index_buffer += length_frame_size;
        }

        if (buffer_length < 1 + length_frame_size + 4 + length_frame)
            return 0;

        /* read masks (4 bytes) */
        int masks[4];
        for (i = 0; i < 4; i++)
        {
            masks[i] = (int)((unsigned char)buffer[index_buffer + i]);
        }
        index_buffer += 4;

        /* decode data using masks */
        for (i = 0; i < length_frame; i++)
        {
            decoded[*decoded_length + i] = (int)((unsigned char)buffer[index_buffer + i]) ^ masks[i % 4];
        }
        decoded[*decoded_length + length_frame] = '\0';
        *decoded_length += length_frame;
        index_buffer += length_frame;
    }

    return 1;
}

/*
 * Encodes data in a websocket frame.
 *
 * Returns websocket frame, NULL if error.
 * Argument "length_frame" is set with the length of frame built.
 *
 * Note: result must be freed after use.
 */

char *
relay_websocket_encode_frame (struct t_relay_client *client,
                              const char *buffer,
                              unsigned long long length,
                              unsigned long long *length_frame)
{
    unsigned char *frame;
    unsigned long long index;

    *length_frame = 0;

    frame = malloc (length + 10);
    if (!frame)
        return NULL;

    frame[0] = (client->send_data_type == RELAY_CLIENT_DATA_TEXT) ? 0x81 : 0x82;

    if (length <= 125)
    {
        /* length on one byte */
        frame[1] = length;
        index = 2;
    }
    else if ((length >= 126) && (length <= 65535))
    {
        /* length on 2 bytes */
        frame[1] = 126;
        frame[2] = (length >> 8) & 0xFF;
        frame[3] = length & 0xFF;
        index = 4;
    }
    else
    {
        /* length on 8 bytes */
        frame[1] = 127;
        frame[2] = (length >> 56) & 0xFF;
        frame[3] = (length >> 48) & 0xFF;
        frame[4] = (length >> 40) & 0xFF;
        frame[5] = (length >> 32) & 0xFF;
        frame[6] = (length >> 24) & 0xFF;
        frame[7] = (length >> 16) & 0xFF;
        frame[8] = (length >> 8) & 0xFF;
        frame[9] = length & 0xFF;
        index = 10;
    }

    /* copy buffer after length */
    memcpy (frame + index, buffer, length);

    *length_frame = index + length;

    return (char *)frame;
}
