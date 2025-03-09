/*
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

#ifndef WEECHAT_PLUGIN_RELAY_WEBSOCKET_H
#define WEECHAT_PLUGIN_RELAY_WEBSOCKET_H

#include "relay.h"

#include <zlib.h>

/*
 * globally unique identifier that is concatenated to HTTP header
 * "Sec-WebSocket-Key"
 */
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define WEBSOCKET_FRAME_OPCODE_CONTINUATION 0x00
#define WEBSOCKET_FRAME_OPCODE_TEXT         0x01
#define WEBSOCKET_FRAME_OPCODE_BINARY       0x02
#define WEBSOCKET_FRAME_OPCODE_CLOSE        0x08
#define WEBSOCKET_FRAME_OPCODE_PING         0x09
#define WEBSOCKET_FRAME_OPCODE_PONG         0x0A

#define WEBSOCKET_SUB_PROTOCOL_API_WEECHAT "api.weechat"

struct t_relay_client;
struct t_relay_http_request;

struct t_relay_websocket_deflate
{
    int enabled;                       /* 1 if permessage-deflate is enabled*/
    int server_context_takeover;       /* context takeover for server       */
    int client_context_takeover;       /* context takeover for client       */
    int window_bits_deflate;           /* window bits for server (comp.)    */
                                       /* ("server_max_window_bits")        */
    int window_bits_inflate;           /* window bits for client (decomp.)  */
                                       /* ("client_max_window_bits")        */
    int server_max_window_bits_recv;   /* "server_max_window_bits" received?*/
    int client_max_window_bits_recv;   /* "client_max_window_bits" received?*/
    z_stream *strm_deflate;            /* stream for deflate (compression)  */
    z_stream *strm_inflate;            /* stream for inflate (decompression)*/
};

struct t_relay_websocket_frame
{
    int opcode;                        /* frame opcode                      */
    int payload_size;                  /* size of payload                   */
    char *payload;                     /* payload                           */
};

extern struct t_relay_websocket_deflate *relay_websocket_deflate_alloc (void);
extern int relay_websocket_deflate_init_stream_deflate (struct t_relay_websocket_deflate *ws_deflate);
extern int relay_websocket_deflate_init_stream_inflate (struct t_relay_websocket_deflate *ws_deflate);
extern void relay_websocket_deflate_reinit (struct t_relay_websocket_deflate *ws_deflate);
extern void relay_websocket_deflate_free (struct t_relay_websocket_deflate *ws_deflate);
extern int relay_websocket_is_valid_http_get (enum t_relay_protocol protocol,
                                              const char *message);
extern int relay_websocket_client_handshake_valid (struct t_relay_http_request *request);
extern void relay_websocket_parse_extensions (const char *extensions,
                                              struct t_relay_websocket_deflate *ws_deflate,
                                              int ws_deflate_allowed);
extern char *relay_websocket_build_handshake (struct t_relay_http_request *request);
extern int relay_websocket_decode_frame (const unsigned char *buffer,
                                         unsigned long long length,
                                         int expect_masked_frame,
                                         struct t_relay_websocket_deflate *ws_deflate,
                                         struct t_relay_websocket_frame **frames,
                                         int *num_frames,
                                         char **partial_ws_frame,
                                         int *partial_ws_frame_size);
extern char *relay_websocket_encode_frame (struct t_relay_websocket_deflate *ws_deflate,
                                           int opcode,
                                           int mask_frame,
                                           const char *payload,
                                           unsigned long long payload_size,
                                           unsigned long long *length_frame);
extern void relay_websocket_deflate_print_log (struct t_relay_websocket_deflate *ws_deflate,
                                               const char *prefix);

#endif /* WEECHAT_PLUGIN_RELAY_WEBSOCKET_H */
