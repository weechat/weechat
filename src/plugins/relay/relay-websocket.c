/*
 * relay-websocket.c - websocket server functions for relay plugin (RFC 6455)
 *
 * Copyright (C) 2013-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include <zlib.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-http.h"
#include "relay-websocket.h"


/*
 * globally unique identifier that is concatenated to HTTP header
 * "Sec-WebSocket-Key"
 */
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


/*
 * Allocates a t_relay_websocket_deflate structure.
 */

struct t_relay_websocket_deflate *
relay_websocket_deflate_alloc ()
{
    struct t_relay_websocket_deflate *new_ws_deflate;

    new_ws_deflate = (struct t_relay_websocket_deflate *)malloc (sizeof (*new_ws_deflate));
    if (!new_ws_deflate)
        return NULL;

    new_ws_deflate->enabled = 0;
    new_ws_deflate->server_context_takeover = 0;
    new_ws_deflate->server_context_takeover = 0;
    new_ws_deflate->window_bits_deflate = 0;
    new_ws_deflate->window_bits_inflate = 0;
    new_ws_deflate->strm_deflate = NULL;
    new_ws_deflate->strm_inflate = NULL;

    return new_ws_deflate;
}

/*
 * Initializes stream for deflate (compression).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_websocket_deflate_init_stream_deflate (struct t_relay_websocket_deflate *ws_deflate)
{
    int rc, compression, compression_level;

    compression = weechat_config_integer (relay_config_network_compression);

    /* convert % to zlib compression level (1-9) */
    compression_level = (((compression - 1) * 9) / 100) + 1;

    rc = deflateInit2 (
        ws_deflate->strm_deflate,
        compression_level,
        Z_DEFLATED,                  /* method */
        -1 * ws_deflate->window_bits_deflate,
        8,                           /* memLevel */
        Z_DEFAULT_STRATEGY);         /* strategy */

    return (rc == Z_OK) ? 1 : 0;
}

/*
 * Frees a deflate stream in a deflate structure.
 */

void
relay_websocket_deflate_free_stream_deflate (struct t_relay_websocket_deflate *ws_deflate)
{
    if (ws_deflate->strm_deflate)
    {
        deflateEnd (ws_deflate->strm_deflate);
        free (ws_deflate->strm_deflate);
        ws_deflate->strm_deflate = NULL;
    }
}

/*
 * Initializes stream for inflate (decompression).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
relay_websocket_deflate_init_stream_inflate (struct t_relay_websocket_deflate *ws_deflate)
{
    int rc;

    rc = inflateInit2 (ws_deflate->strm_inflate,
                       -1 * ws_deflate->window_bits_inflate);

    return (rc == Z_OK) ? 1 : 0;
}

/*
 * Frees an inflate stream in a deflate structure.
 */

void
relay_websocket_deflate_free_stream_inflate (struct t_relay_websocket_deflate *ws_deflate)
{
    if (ws_deflate->strm_inflate)
    {
        inflateEnd (ws_deflate->strm_inflate);
        free (ws_deflate->strm_inflate);
        ws_deflate->strm_inflate = NULL;
    }
}

/*
 * Frees a websocket deflate structure.
 */

void
relay_websocket_deflate_free (struct t_relay_websocket_deflate *ws_deflate)
{
    relay_websocket_deflate_free_stream_deflate (ws_deflate);
    relay_websocket_deflate_free_stream_inflate (ws_deflate);
    free (ws_deflate);
}

/*
 * Checks if a message is a HTTP GET with resource "/weechat" (for weechat
 * protocol) or "/api" (for api protocol).
 *
 * Returns:
 *   1: message is a HTTP GET with appropriate resource
 *   0: message is NOT a HTTP GET with appropriate resource
 */

int
relay_websocket_is_valid_http_get (enum t_relay_protocol protocol,
                                   const char *message)
{
    char string[128];
    int length;

    if (!message)
        return 0;

    /* the message must start with "GET /weechat" or "GET /api" */
    snprintf (string, sizeof (string),
              "GET /%s", relay_protocol_string[protocol]);
    length = strlen (string);

    if (strncmp (message, string, length) != 0)
        return 0;

    /* after "GET /weechat" or "GET /api", only a new line or " HTTP" is allowed */
    if ((message[length] != '\r') && (message[length] != '\n')
        && (strncmp (message + length, " HTTP", 5) != 0))
    {
        return 0;
    }

    /* valid HTTP GET */
    return 1;
}

/*
 * Checks if a client handshake is valid.
 *
 * A websocket query looks like (weechat protocol):
 *   GET /weechat HTTP/1.1
 *   Upgrade: websocket
 *   Connection: Upgrade
 *   Host: myhost:5000
 *   Origin: https://example.org
 *   Pragma: no-cache
 *   Cache-Control: no-cache
 *   Sec-WebSocket-Key: fo1J9uHSsrfDP3BkwUylzQ==
 *   Sec-WebSocket-Version: 13
 *   Sec-WebSocket-Extensions: x-webkit-deflate-frame
 *   Cookie: csrftoken=acb65377798f32dc377ebb50316a12b5
 *
 * A websocket query looks like (api protocol):
 *   GET /api HTTP/1.1
 *   Upgrade: websocket
 *   Connection: Upgrade
 *   Host: myhost:5000
 *   Origin: https://example.org
 *   Pragma: no-cache
 *   Cache-Control: no-cache
 *   Sec-WebSocket-Key: fo1J9uHSsrfDP3BkwUylzQ==
 *   Sec-WebSocket-Version: 13
 *   Sec-WebSocket-Extensions: x-webkit-deflate-frame
 *   Cookie: csrftoken=acb65377798f32dc377ebb50316a12b5
 *
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
relay_websocket_client_handshake_valid (struct t_relay_http_request *request)
{
    const char *value;

    if (!request || !request->headers)
        return -1;

    /* check if we have header "Upgrade" with value "websocket" */
    value = weechat_hashtable_get (request->headers, "upgrade");
    if (!value)
        return -1;
    if (weechat_strcasecmp (value, "websocket") != 0)
        return -1;

    /* check if we have header "Sec-WebSocket-Key" with non-empty value */
    value = weechat_hashtable_get (request->headers, "sec-websocket-key");
    if (!value || !value[0])
        return -1;

    if (relay_config_regex_websocket_allowed_origins)
    {
        value = weechat_hashtable_get (request->headers, "origin");
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
 * Parses websocket extensions (header "Sec-WebSocket-Extensions").
 *
 * Header is for example:
 *   Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
 */

void
relay_websocket_parse_extensions (const char *extensions,
                                  struct t_relay_websocket_deflate *ws_deflate)
{
    char **exts, **params, **items, *error;
    int i, j, num_exts, num_params, num_items;
    long number;

    if (!extensions || !ws_deflate)
        return;

    exts = weechat_string_split (extensions, ",", " ", 0, 0, &num_exts);
    if (!exts)
        return;

    for (i = 0; i < num_exts; i++)
    {
        params = weechat_string_split (exts[i], ";", " ", 0, 0, &num_params);
        if (params && (num_params >= 1)
            && (strcmp (params[0], "permessage-deflate") == 0))
        {
            ws_deflate->enabled = 1;
            ws_deflate->server_context_takeover = 1;
            ws_deflate->client_context_takeover = 1;
            ws_deflate->window_bits_deflate = 15;
            ws_deflate->window_bits_inflate = 15;
            for (j = 1; j < num_params; j++)
            {
                items = weechat_string_split (params[j], "=", " ", 0, 0, &num_items);
                if (items && (num_items >= 1))
                {
                    if (strcmp (items[0], "server_no_context_takeover") == 0)
                    {
                        ws_deflate->server_context_takeover = 0;
                    }
                    else if (strcmp (items[0], "client_no_context_takeover") == 0)
                    {
                        ws_deflate->client_context_takeover = 0;
                    }
                    else if ((strcmp (items[0], "server_max_window_bits") == 0)
                             || (strcmp (items[0], "client_max_window_bits") == 0))
                    {
                        number = 15;
                        if (num_items >= 2)
                        {
                            error = NULL;
                            number = strtol (items[1], &error, 10);
                            if (error && !error[0])
                            {
                                if (number < 8)
                                    number = 8;
                                else if (number > 15)
                                    number = 15;
                            }
                            else
                            {
                                number = 15;
                            }
                        }
                        if (strcmp (items[0], "server_max_window_bits") == 0)
                            ws_deflate->window_bits_deflate = (int)number;
                        else
                            ws_deflate->window_bits_inflate = (int)number;
                    }
                }
                if (items)
                    weechat_string_free_split (items);
            }
        }
        if (params)
            weechat_string_free_split (params);
    }

    weechat_string_free_split (exts);
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
relay_websocket_build_handshake (struct t_relay_http_request *request)
{
    const char *sec_websocket_key;
    char *key, sec_websocket_accept[128], handshake[4096], hash[160 / 8];
    char sec_websocket_extensions[512];
    int length, hash_size;

    if (!request)
        return NULL;

    sec_websocket_key = weechat_hashtable_get (request->headers,
                                               "sec-websocket-key");
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
    if (!weechat_crypto_hash (key, strlen (key), "sha1", hash, &hash_size))
    {
        free (key);
        return NULL;
    }
    if (weechat_string_base_encode ("64", hash, hash_size,
                                    sec_websocket_accept) < 0)
    {
        sec_websocket_accept[0] = '\0';
    }

    free (key);

    if (request->ws_deflate->enabled)
    {
        snprintf (
            sec_websocket_extensions, sizeof (sec_websocket_extensions),
            "Sec-WebSocket-Extensions: permessage-deflate; "
            "%s"
            "%s"
            "server_max_window_bits=%d; "
            "client_max_window_bits=%d\r\n",
            (!request->ws_deflate->server_context_takeover) ? "server_no_context_takeover; " : "",
            (!request->ws_deflate->client_context_takeover) ? "client_no_context_takeover; " : "",
            request->ws_deflate->window_bits_deflate,
            request->ws_deflate->window_bits_inflate);
    }
    else
    {
        sec_websocket_extensions[0] = '\0';
    }

    /* build the handshake (it will be sent as-is to client) */
    snprintf (handshake, sizeof (handshake),
              "HTTP/1.1 101 Switching Protocols\r\n"
              "Upgrade: websocket\r\n"
              "Connection: Upgrade\r\n"
              "Sec-WebSocket-Accept: %s\r\n"
              "%s"
              "\r\n",
              sec_websocket_accept,
              sec_websocket_extensions);

    return strdup (handshake);
}

/*
 * Decompresses a decoded and compressed websocket frame compressed with
 * "deflate" (when websocket extension "permessage-deflate" is enabled).
 *
 * A final '\0' is added after the decompressed data (the size_decompressed
 * does not count this final '\0').
 *
 * Returns pointer to decompressed data, NULL if error.
 */

char *
relay_websocket_inflate (const void *data, size_t size, z_stream *strm,
                         size_t *size_decompressed)
{
    int rc;
    unsigned char append_bytes[4] = { 0x00, 0x00, 0xFF, 0xFF };
    Bytef *data2, *dest, *dest2;
    uLongf size2, dest_size;

    if (!data || (size == 0) || !strm || !size_decompressed)
        return NULL;

    dest = NULL;

    *size_decompressed = 0;

    /* append "0x00 0x00 0xFF 0xFF" to data */
    size2 = size + sizeof (append_bytes);
    data2 = malloc (size2);
    if (!data2)
        goto error;
    memcpy (data2, data, size);
    memcpy (data2 + size, append_bytes, sizeof (append_bytes));

    /* estimate the decompressed size, by default 10 * size */
    dest_size = 10 * size2;
    dest = malloc (dest_size);
    if (!dest)
        goto error;

    strm->avail_in = (uInt)size2;
    strm->next_in = (Bytef *)data2;
    strm->total_in = 0;

    strm->avail_out = (uInt)dest_size;
    strm->next_out = (Bytef *)dest;
    strm->total_out = 0;

    /* loop until we manage to decompress whole data in dest */
    while (1)
    {
        rc = inflate (strm, Z_SYNC_FLUSH);
        if ((rc == Z_STREAM_END) || (rc == Z_OK))
        {
            /* data successfully decompressed */
            *size_decompressed = strm->total_out;
            break;
        }
        else if (rc == Z_BUF_ERROR)
        {
            strm->avail_out += dest_size;
            dest_size *= 2;
            dest2 = realloc (dest, dest_size);
            if (!dest2)
                goto error;
            dest = dest2;
            strm->next_out = dest + strm->total_out;
        }
        else
        {
            /* any other error is fatal */
            goto error;
        }
    }

    dest2 = realloc (dest, *size_decompressed + 1);
    if (!dest2)
        goto error;
    dest = dest2;
    dest[*size_decompressed] = '\0';
    if (data2)
        free (data2);
    return (char *)dest;

error:
    if (data2)
        free (data2);
    if (dest)
        free (dest);
    return NULL;
}

/*
 * Decodes a websocket frame and return a list of frames in "*frames" (each
 * frame is first decompressed if "permessage-deflate" websocket extension
 * is used).
 *
 * Returns:
 *   1: frame decoded successfully
 *   0: error decoding frame (connection must be closed if it happens)
 */

int
relay_websocket_decode_frame (struct t_relay_client *client,
                              const unsigned char *buffer,
                              unsigned long long buffer_length,
                              struct t_relay_websocket_frame **frames,
                              int *num_frames)
{
    unsigned long long i, index_buffer, length_frame_size, length_frame;
    unsigned char opcode;
    size_t size_decompressed;
    char *payload_decompressed;
    struct t_relay_websocket_frame *frames2, *ptr_frame;

    if (!buffer || !frames || !num_frames)
        return 0;

    *frames = NULL;
    *num_frames = 0;

    index_buffer = 0;

    /* loop to decode all frames in message */
    while (index_buffer + 1 < buffer_length)
    {
        (*num_frames)++;

        frames2 = realloc (*frames, sizeof (**frames) * (*num_frames));
        if (!frames2)
            return 0;
        *frames = frames2;

        ptr_frame = &((*frames)[*num_frames - 1]);

        ptr_frame->opcode = 0;
        ptr_frame->payload_size = 0;
        ptr_frame->payload = NULL;

        opcode = buffer[index_buffer] & 15;

        /*
         * check if frame is masked: client MUST send a masked frame; if frame is
         * not masked, we MUST reject it and close the connection (see RFC 6455)
         */
        if (!(buffer[index_buffer + 1] & 128))
            return 0;

        /* decode frame */
        length_frame = buffer[index_buffer + 1] & 127;
        index_buffer += 2;
        if (index_buffer >= buffer_length)
            return 0;
        if ((length_frame == 126) || (length_frame == 127))
        {
            length_frame_size = (length_frame == 126) ? 2 : 8;
            if (index_buffer + length_frame_size > buffer_length)
                return 0;
            length_frame = 0;
            for (i = 0; i < length_frame_size; i++)
            {
                length_frame += (unsigned long long)buffer[index_buffer + i] << ((length_frame_size - i - 1) * 8);
            }
            index_buffer += length_frame_size;
        }

        /* read masks (4 bytes) */
        if (index_buffer + 4 > buffer_length)
            return 0;
        int masks[4];
        for (i = 0; i < 4; i++)
        {
            masks[i] = (int)((unsigned char)buffer[index_buffer + i]);
        }
        index_buffer += 4;

        /* save opcode */
        switch (opcode)
        {
            case WEBSOCKET_FRAME_OPCODE_PING:
                ptr_frame->opcode = RELAY_CLIENT_MSG_PING;
                break;
            case WEBSOCKET_FRAME_OPCODE_CLOSE:
                ptr_frame->opcode = RELAY_CLIENT_MSG_CLOSE;
                break;
            default:
                ptr_frame->opcode = RELAY_CLIENT_MSG_STANDARD;
                break;
        }

        /* decode data using masks */
        if ((length_frame > buffer_length)
            || (index_buffer + length_frame > buffer_length))
        {
            return 0;
        }

        ptr_frame->payload = malloc (length_frame + 1);
        if (!ptr_frame->payload)
            return 0;
        ptr_frame->payload_size = length_frame;

        /* fill payload */
        for (i = 0; i < length_frame; i++)
        {
            ptr_frame->payload[i] = (int)((unsigned char)buffer[index_buffer + i]) ^ masks[i % 4];
        }
        ptr_frame->payload[length_frame] = '\0';

        /*
         * decompress data if frame is not empty and if "permessage-deflate"
         * is enabled
         */
        if ((length_frame > 0) && client->ws_deflate->enabled)
        {
            if (!client->ws_deflate->strm_inflate)
            {
                client->ws_deflate->strm_inflate = calloc (
                    1, sizeof (*client->ws_deflate->strm_inflate));
                if (!client->ws_deflate->strm_inflate)
                    return 0;
                if (!relay_websocket_deflate_init_stream_inflate (client->ws_deflate))
                    return 0;
            }
            payload_decompressed = relay_websocket_inflate (
                ptr_frame->payload,
                ptr_frame->payload_size,
                client->ws_deflate->strm_inflate,
                &size_decompressed);
            if (!payload_decompressed)
                return 0;
            free (ptr_frame->payload);
            ptr_frame->payload = payload_decompressed;
            ptr_frame->payload_size = size_decompressed;
            if (!client->ws_deflate->client_context_takeover)
                relay_websocket_deflate_free_stream_inflate (client->ws_deflate);
        }

        index_buffer += length_frame;
    }

    return 1;
}

/*
 * Compresses data to send in a websocket frame (when websocket extension
 * "permessage-deflate" is enabled).
 *
 * Returns pointer to compressed data, NULL if error.
 */

char *
relay_websocket_deflate (const void *data, size_t size, z_stream *strm,
                         size_t *size_compressed)
{
    int rc;
    uLongf dest_size;
    Bytef *dest;

    if (!data || (size == 0) || !strm || !size_compressed)
        return NULL;

    *size_compressed = 0;

    dest_size = compressBound (size);
    dest = malloc (dest_size);
    if (!dest)
        return NULL;

    strm->avail_in = (uInt)size;
    strm->next_in = (Bytef *)data;
    strm->total_in = 0;

    strm->avail_out = (uInt)dest_size;
    strm->next_out = (Bytef *)dest;
    strm->total_out = 0;

    rc = deflate (strm, Z_SYNC_FLUSH);
    if ((rc == Z_STREAM_END) || (rc == Z_OK))
    {
        *size_compressed = strm->total_out;
        return (char *)dest;
    }

    free (dest);
    return NULL;
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
                              int opcode,
                              const char *payload,
                              unsigned long long payload_size,
                              unsigned long long *length_frame)
{
    const char *ptr_data;
    char *payload_compressed;
    size_t size_compressed;
    unsigned char *frame;
    unsigned long long index, data_size;

    *length_frame = 0;

    ptr_data = payload;
    data_size = payload_size;

    payload_compressed = NULL;
    size_compressed = 0;

    /*
     * compress data if payload is not empty and if "permessage-deflate"
     * is enabled
     */
    if (((opcode == WEBSOCKET_FRAME_OPCODE_TEXT)
         || (opcode == WEBSOCKET_FRAME_OPCODE_BINARY))
        && (payload_size > 0)
        && client->ws_deflate->enabled)
    {
        if (!client->ws_deflate->strm_deflate)
        {
            client->ws_deflate->strm_deflate = calloc (
                1, sizeof (*client->ws_deflate->strm_deflate));
            if (!client->ws_deflate->strm_deflate)
                return NULL;
            if (!relay_websocket_deflate_init_stream_deflate (client->ws_deflate))
                return NULL;
        }
        payload_compressed = relay_websocket_deflate (
            payload,
            payload_size,
            client->ws_deflate->strm_deflate,
            &size_compressed);
        if (!payload_compressed)
            return NULL;
        ptr_data = payload_compressed;
        data_size = size_compressed;
        if ((data_size > 4)
            && ((unsigned char)ptr_data[data_size - 4] == 0x00)
            && ((unsigned char)ptr_data[data_size - 3] == 0x00)
            && ((unsigned char)ptr_data[data_size - 2] == 0xFF)
            && ((unsigned char)ptr_data[data_size - 1] == 0xFF))
        {
            data_size -= 4;
        }
        if (!client->ws_deflate->server_context_takeover)
            relay_websocket_deflate_free_stream_deflate (client->ws_deflate);
        /* set bit RSV1: indicate permessage-deflate compressed data */
        opcode |= 0x40;
    }

    frame = malloc (data_size + 10);
    if (!frame)
    {
        if (payload_compressed)
            free (payload_compressed);
        return NULL;
    }

    frame[0] = 0x80;
    frame[0] |= opcode;

    if (data_size <= 125)
    {
        /* length on one byte */
        frame[1] = data_size;
        index = 2;
    }
    else if (data_size <= 65535)
    {
        /* length on 2 bytes */
        frame[1] = 126;
        frame[2] = (data_size >> 8) & 0xFF;
        frame[3] = data_size & 0xFF;
        index = 4;
    }
    else
    {
        /* length on 8 bytes */
        frame[1] = 127;
        frame[2] = (data_size >> 56) & 0xFF;
        frame[3] = (data_size >> 48) & 0xFF;
        frame[4] = (data_size >> 40) & 0xFF;
        frame[5] = (data_size >> 32) & 0xFF;
        frame[6] = (data_size >> 24) & 0xFF;
        frame[7] = (data_size >> 16) & 0xFF;
        frame[8] = (data_size >> 8) & 0xFF;
        frame[9] = data_size & 0xFF;
        index = 10;
    }

    /* copy buffer after data_size */
    memcpy (frame + index, ptr_data, data_size);

    *length_frame = index + data_size;

    if (payload_compressed)
        free (payload_compressed);

    return (char *)frame;
}

/*
 * Prints websocket deflate data in WeeChat log file (usually for crash dump).
 */

void
relay_websocket_deflate_print_log (struct t_relay_websocket_deflate *ws_deflate,
                                   const char *prefix)
{
    weechat_log_printf ("%s  ws_deflate:", prefix);
    weechat_log_printf ("%s    enabled . . . . . . . . : %d", prefix, ws_deflate->enabled);
    weechat_log_printf ("%s    server_context_takeover : %d", prefix, ws_deflate->server_context_takeover);
    weechat_log_printf ("%s    client_context_takeover : %d", prefix, ws_deflate->client_context_takeover);
    weechat_log_printf ("%s    window_bits_deflate . . : %d", prefix, ws_deflate->window_bits_deflate);
    weechat_log_printf ("%s    window_bits_inflate . . : %d", prefix, ws_deflate->window_bits_inflate);
    weechat_log_printf ("%s    strm_deflate. . . . . . : 0x%lx", prefix, ws_deflate->strm_deflate);
    weechat_log_printf ("%s    strm_inflate. . . . . . : 0x%lx", prefix, ws_deflate->strm_inflate);
}
