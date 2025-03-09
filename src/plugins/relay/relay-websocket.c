/*
 * relay-websocket.c - websocket server functions for relay plugin (RFC 6455)
 *
 * Copyright (C) 2013-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <gcrypt.h>
#include <zlib.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-client.h"
#include "relay-config.h"
#include "relay-http.h"
#include "relay-websocket.h"


/*
 * Allocates a t_relay_websocket_deflate structure.
 */

struct t_relay_websocket_deflate *
relay_websocket_deflate_alloc (void)
{
    struct t_relay_websocket_deflate *new_ws_deflate;

    new_ws_deflate = (struct t_relay_websocket_deflate *)malloc (sizeof (*new_ws_deflate));
    if (!new_ws_deflate)
        return NULL;

    new_ws_deflate->enabled = 0;
    new_ws_deflate->server_context_takeover = 0;
    new_ws_deflate->client_context_takeover = 0;
    new_ws_deflate->window_bits_deflate = 0;
    new_ws_deflate->window_bits_inflate = 0;
    new_ws_deflate->server_max_window_bits_recv = 0;
    new_ws_deflate->client_max_window_bits_recv = 0;
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
 * Reinitializes a t_relay_websocket_deflate structure.
 */

void
relay_websocket_deflate_reinit (struct t_relay_websocket_deflate *ws_deflate)
{
    ws_deflate->enabled = 0;
    ws_deflate->server_context_takeover = 0;
    ws_deflate->client_context_takeover = 0;
    ws_deflate->window_bits_deflate = 0;
    ws_deflate->window_bits_inflate = 0;
    ws_deflate->server_max_window_bits_recv = 0;
    ws_deflate->client_max_window_bits_recv = 0;
    relay_websocket_deflate_free_stream_deflate (ws_deflate);
    relay_websocket_deflate_free_stream_inflate (ws_deflate);
}

/*
 * Frees a websocket deflate structure.
 */

void
relay_websocket_deflate_free (struct t_relay_websocket_deflate *ws_deflate)
{
    if (ws_deflate)
    {
        relay_websocket_deflate_free_stream_deflate (ws_deflate);
        relay_websocket_deflate_free_stream_inflate (ws_deflate);
        free (ws_deflate);
    }
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
                                  struct t_relay_websocket_deflate *ws_deflate,
                                  int ws_deflate_allowed)
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
            && (strcmp (params[0], "permessage-deflate") == 0)
            && ws_deflate_allowed
            && (weechat_config_boolean (relay_config_network_websocket_permessage_deflate)))
        {
            ws_deflate->enabled = 1;
            ws_deflate->server_context_takeover = 1;
            ws_deflate->client_context_takeover = 1;
            ws_deflate->window_bits_deflate = 15;
            ws_deflate->window_bits_inflate = 15;
            ws_deflate->server_max_window_bits_recv = 0;
            ws_deflate->client_max_window_bits_recv = 0;
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
                        {
                            ws_deflate->server_max_window_bits_recv = 1;
                            ws_deflate->window_bits_deflate = (int)number;
                        }
                        else
                        {
                            ws_deflate->client_max_window_bits_recv = 1;
                            ws_deflate->window_bits_inflate = (int)number;
                        }
                    }
                }
                weechat_string_free_split (items);
            }
        }
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
    const char *sec_websocket_key, *sec_websocket_protocol_request;
    char *key, sec_websocket_accept[128], handshake[4096], hash[160 / 8];
    char **extensions, **protocol_array, str_window_bits[128];
    char sec_websocket_extensions[1024];
    char sec_websocket_protocol[1024];
    int i, hash_size, protocol_count;

    if (!request)
        return NULL;

    sec_websocket_key = weechat_hashtable_get (request->headers,
                                               "sec-websocket-key");
    if (!sec_websocket_key || !sec_websocket_key[0])
        return NULL;

    /*
     * concatenate header "Sec-WebSocket-Key" with the GUID
     * (globally unique identifier)
     */
    if (weechat_asprintf (&key, "%s%s", sec_websocket_key, WEBSOCKET_GUID) < 0)
        return NULL;

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
        extensions = weechat_string_dyn_alloc (128);
        if (!extensions)
            return NULL;
        weechat_string_dyn_concat (extensions, "permessage-deflate", -1);
        if (!request->ws_deflate->server_context_takeover)
        {
            weechat_string_dyn_concat (extensions, "; ", -1);
            weechat_string_dyn_concat (extensions, "server_no_context_takeover", -1);
        }
        if (!request->ws_deflate->client_context_takeover)
        {
            weechat_string_dyn_concat (extensions, "; ", -1);
            weechat_string_dyn_concat (extensions, "client_no_context_takeover", -1);
        }
        if (request->ws_deflate->server_max_window_bits_recv)
        {
            weechat_string_dyn_concat (extensions, "; ", -1);
            snprintf (str_window_bits, sizeof (str_window_bits),
                      "server_max_window_bits=%d",
                      request->ws_deflate->window_bits_deflate);
            weechat_string_dyn_concat (extensions, str_window_bits, -1);
        }
        if (request->ws_deflate->client_max_window_bits_recv)
        {
            weechat_string_dyn_concat (extensions, "; ", -1);
            snprintf (str_window_bits, sizeof (str_window_bits),
                      "client_max_window_bits=%d",
                      request->ws_deflate->window_bits_inflate);
            weechat_string_dyn_concat (extensions, str_window_bits, -1);
        }
        snprintf (
            sec_websocket_extensions, sizeof (sec_websocket_extensions),
            "Sec-WebSocket-Extensions: %s\r\n",
            *extensions);
        weechat_string_dyn_free (extensions, 1);
    }
    else
    {
        sec_websocket_extensions[0] = '\0';
    }

    sec_websocket_protocol_request = weechat_hashtable_get (
        request->headers, "sec-websocket-protocol");
    protocol_array = weechat_string_split (sec_websocket_protocol_request,
                                           ",", " ", 0, 0, &protocol_count);

    sec_websocket_protocol[0] = '\0';
    for (i = 0; i < protocol_count; i++)
    {
        if (strcmp (protocol_array[i], WEBSOCKET_SUB_PROTOCOL_API_WEECHAT) == 0)
        {
            snprintf (
                sec_websocket_protocol, sizeof (sec_websocket_protocol),
                "Sec-WebSocket-Protocol: %s\r\n",
                WEBSOCKET_SUB_PROTOCOL_API_WEECHAT);
            break;
        }
    }

    /* build the handshake (it will be sent as-is to client) */
    snprintf (handshake, sizeof (handshake),
              "HTTP/1.1 101 Switching Protocols\r\n"
              "Upgrade: websocket\r\n"
              "Connection: Upgrade\r\n"
              "Sec-WebSocket-Accept: %s\r\n"
              "%s"
              "%s"
              "\r\n",
              sec_websocket_accept,
              sec_websocket_extensions,
              sec_websocket_protocol);

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
        if (((rc == Z_STREAM_END) || (rc == Z_OK))
            && (strm->avail_in == 0))
        {
            /* data successfully decompressed */
            *size_decompressed = strm->total_out;
            break;
        }
        if ((rc == Z_BUF_ERROR)
            || (((rc == Z_STREAM_END) || (rc == Z_OK))
                && (strm->avail_in > 0)))
        {
            /* output buffer is not large enough */
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
    free (data2);
    return (char *)dest;

error:
    free (data2);
    free (dest);
    return NULL;
}

/*
 * Decodes a websocket frame and return a list of frames in "*frames" (each
 * frame is first decompressed if "permessage-deflate" websocket extension
 * is used).
 *
 * If argument "expect_masked_frame" is 1 and a frame is not masked,
 * the function returns an error.
 *
 * Returns:
 *   1: frame(s) decoded successfully
 *   0: error decoding frame (connection must be closed if it happens)
 */

int
relay_websocket_decode_frame (const unsigned char *buffer,
                              unsigned long long buffer_length,
                              int expect_masked_frame,
                              struct t_relay_websocket_deflate *ws_deflate,
                              struct t_relay_websocket_frame **frames,
                              int *num_frames,
                              char **partial_ws_frame,
                              int *partial_ws_frame_size)
{
    unsigned long long i, index_buffer, index_buffer_start_frame;
    unsigned long long length_frame_size, length_frame;
    unsigned char opcode;
    size_t size_decompressed;
    char *payload_decompressed;
    struct t_relay_websocket_frame *frames2, *ptr_frame;
    int size, masked_frame, mask[4];

    if (!buffer || !frames || !num_frames)
        return 0;

    *frames = NULL;
    *num_frames = 0;

    index_buffer = 0;
    index_buffer_start_frame = 0;

    /* loop to decode all frames in message */
    while (index_buffer < buffer_length)
    {
        index_buffer_start_frame = index_buffer;

        if (index_buffer + 1 >= buffer_length)
            goto missing_data;

        opcode = buffer[index_buffer] & 15;

        /* check if frame is masked */
        masked_frame = (buffer[index_buffer + 1] & 128) ? 1 : 0;

        /*
         * error if the frame is not masked and we expect it to be masked,
         * in this case we must reject it and close the connection
         * (see RFC 6455)
         */
        if (!masked_frame && expect_masked_frame)
            return 0;

        /* decode frame length */
        length_frame = buffer[index_buffer + 1] & 127;
        index_buffer += 2;
        if (index_buffer >= buffer_length)
            goto missing_data;
        if ((length_frame == 126) || (length_frame == 127))
        {
            length_frame_size = (length_frame == 126) ? 2 : 8;
            if (index_buffer + length_frame_size > buffer_length)
                goto missing_data;
            length_frame = 0;
            for (i = 0; i < length_frame_size; i++)
            {
                length_frame += (unsigned long long)buffer[index_buffer + i] << ((length_frame_size - i - 1) * 8);
            }
            index_buffer += length_frame_size;
        }

        if (masked_frame)
        {
            /* read mask (4 bytes) */
            if (index_buffer + 4 > buffer_length)
                goto missing_data;
            for (i = 0; i < 4; i++)
            {
                mask[i] = (int)((unsigned char)buffer[index_buffer + i]);
            }
            index_buffer += 4;
        }

        /* check if we have enough data */
        if ((length_frame > buffer_length)
            || (index_buffer + length_frame > buffer_length))
        {
            goto missing_data;
        }

        /* add a new frame in array */
        (*num_frames)++;

        frames2 = realloc (*frames, sizeof (**frames) * (*num_frames));
        if (!frames2)
            return 0;
        *frames = frames2;

        ptr_frame = &((*frames)[*num_frames - 1]);

        ptr_frame->opcode = 0;
        ptr_frame->payload_size = 0;
        ptr_frame->payload = NULL;

        /* save opcode */
        switch (opcode)
        {
            case WEBSOCKET_FRAME_OPCODE_PING:
                ptr_frame->opcode = RELAY_MSG_PING;
                break;
            case WEBSOCKET_FRAME_OPCODE_CLOSE:
                ptr_frame->opcode = RELAY_MSG_CLOSE;
                break;
            default:
                ptr_frame->opcode = RELAY_MSG_STANDARD;
                break;
        }

        /* allocate payload */
        ptr_frame->payload = malloc (length_frame + 1);
        if (!ptr_frame->payload)
            return 0;
        ptr_frame->payload_size = length_frame;

        /* fill payload */
        if (masked_frame)
        {
            for (i = 0; i < length_frame; i++)
            {
                ptr_frame->payload[i] = (int)((unsigned char)buffer[index_buffer + i]) ^ mask[i % 4];
            }
        }
        else
        {
            memcpy (ptr_frame->payload, buffer + index_buffer, length_frame);
        }
        ptr_frame->payload[length_frame] = '\0';

        /*
         * decompress data if frame is not empty and if "permessage-deflate"
         * is enabled
         */
        if ((length_frame > 0) && ws_deflate && ws_deflate->enabled)
        {
            if (!ws_deflate->strm_inflate)
            {
                ws_deflate->strm_inflate = calloc (
                    1, sizeof (*ws_deflate->strm_inflate));
                if (!ws_deflate->strm_inflate)
                    return 0;
                if (!relay_websocket_deflate_init_stream_inflate (ws_deflate))
                    return 0;
            }
            payload_decompressed = relay_websocket_inflate (
                ptr_frame->payload,
                ptr_frame->payload_size,
                ws_deflate->strm_inflate,
                &size_decompressed);
            if (!payload_decompressed)
                return 0;
            free (ptr_frame->payload);
            ptr_frame->payload = payload_decompressed;
            ptr_frame->payload_size = size_decompressed;
            if (!ws_deflate->client_context_takeover)
                relay_websocket_deflate_free_stream_inflate (ws_deflate);
        }

        index_buffer += length_frame;
    }

    if (*partial_ws_frame)
    {
        free (*partial_ws_frame);
        *partial_ws_frame = NULL;
        *partial_ws_frame_size = 0;
    }

    return 1;

missing_data:
    if (*partial_ws_frame)
    {
        free (*partial_ws_frame);
        *partial_ws_frame = NULL;
        *partial_ws_frame_size = 0;
    }
    size = buffer_length - index_buffer_start_frame;
    if (size >= 0)
    {
        *partial_ws_frame = malloc (size);
        if (!*partial_ws_frame)
            return 0;
        memcpy (*partial_ws_frame, buffer + index_buffer_start_frame, size);
        *partial_ws_frame_size = size;
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
 * Argument "mask_frame" must be 1 when sending to server (remote) and 0 when
 * sending to a client.
 *
 * Note: result must be freed after use.
 */

char *
relay_websocket_encode_frame (struct t_relay_websocket_deflate *ws_deflate,
                              int opcode,
                              int mask_frame,
                              const char *payload,
                              unsigned long long payload_size,
                              unsigned long long *length_frame)
{
    const char *ptr_data;
    char *payload_compressed;
    size_t size_compressed;
    unsigned char *frame, *ptr_mask;
    unsigned long long i, index, data_size;

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
        && ws_deflate
        && ws_deflate->enabled)
    {
        if (!ws_deflate->strm_deflate)
        {
            ws_deflate->strm_deflate = calloc (
                1, sizeof (*ws_deflate->strm_deflate));
            if (!ws_deflate->strm_deflate)
                return NULL;
            if (!relay_websocket_deflate_init_stream_deflate (ws_deflate))
                return NULL;
        }
        payload_compressed = relay_websocket_deflate (
            payload,
            payload_size,
            ws_deflate->strm_deflate,
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
        if (!ws_deflate->server_context_takeover)
            relay_websocket_deflate_free_stream_deflate (ws_deflate);
        /* set bit RSV1: indicate permessage-deflate compressed data */
        opcode |= 0x40;
    }

    frame = malloc (data_size + 14);
    if (!frame)
    {
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

    if (mask_frame)
    {
        frame[1] |= 128;
        ptr_mask = frame + index;
        gcry_create_nonce (ptr_mask, 4);
        index += 4;
    }

    /* copy buffer after data_size */
    memcpy (frame + index, ptr_data, data_size);

    /* mask frame */
    if (mask_frame)
    {
        for (i = 0; i < data_size; i++)
        {
            frame[index + i] = frame[index + i] ^ ptr_mask[i % 4];
        }
    }

    *length_frame = index + data_size;

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
    weechat_log_printf ("%s    enabled. . . . . . . . . . : %d", prefix, ws_deflate->enabled);
    weechat_log_printf ("%s    server_context_takeover. . : %d", prefix, ws_deflate->server_context_takeover);
    weechat_log_printf ("%s    client_context_takeover. . : %d", prefix, ws_deflate->client_context_takeover);
    weechat_log_printf ("%s    window_bits_deflate. . . . : %d", prefix, ws_deflate->window_bits_deflate);
    weechat_log_printf ("%s    window_bits_inflate. . . . : %d", prefix, ws_deflate->window_bits_inflate);
    weechat_log_printf ("%s    server_max_window_bits_recv: %d", prefix, ws_deflate->server_max_window_bits_recv);
    weechat_log_printf ("%s    client_max_window_bits_recv: %d", prefix, ws_deflate->client_max_window_bits_recv);
    weechat_log_printf ("%s    strm_deflate . . . . . . . : %p", prefix, ws_deflate->strm_deflate);
    weechat_log_printf ("%s    strm_inflate . . . . . . . : %p", prefix, ws_deflate->strm_inflate);
}
