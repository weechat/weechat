/*
 * Copyright (C) 2013-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#define WEBSOCKET_FRAME_OPCODE_CONTINUATION 0x00
#define WEBSOCKET_FRAME_OPCODE_TEXT         0x01
#define WEBSOCKET_FRAME_OPCODE_BINARY       0x02
#define WEBSOCKET_FRAME_OPCODE_CLOSE        0x08
#define WEBSOCKET_FRAME_OPCODE_PING         0x09
#define WEBSOCKET_FRAME_OPCODE_PONG         0x0A

extern int relay_websocket_is_http_get_weechat (const char *message);
extern void relay_websocket_save_header (struct t_relay_client *client,
                                         const char *message);
extern int relay_websocket_client_handshake_valid (struct t_relay_client *client);
extern char *relay_websocket_build_handshake (struct t_relay_client *client);
extern void relay_websocket_send_http (struct t_relay_client *client,
                                       const char *http);
extern int relay_websocket_decode_frame (const unsigned char *buffer,
                                         unsigned long long length,
                                         unsigned char *decoded,
                                         unsigned long long *decoded_length);
extern char *relay_websocket_encode_frame (int opcode,
                                           const char *buffer,
                                           unsigned long long length,
                                           unsigned long long *length_frame);

#endif /* WEECHAT_PLUGIN_RELAY_WEBSOCKET_H */
