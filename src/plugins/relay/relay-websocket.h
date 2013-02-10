/*
 * Copyright (C) 2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_RELAY_WEBSOCKET_H
#define __WEECHAT_RELAY_WEBSOCKET_H 1

extern int relay_websocket_is_http_get_weechat (const char *message);
extern void relay_websocket_save_header (struct t_relay_client *client,
                                         const char *message);
extern int relay_websocket_client_handshake_valid (struct t_relay_client *client);
extern char *relay_websocket_build_handshake (struct t_relay_client *client);
extern void relay_websocket_send_http (struct t_relay_client *client,
                                       const char *http);
extern int relay_websocket_decode_frame (const unsigned char *buffer,
                                         int length,
                                         unsigned char *decoded);
extern char *relay_websocket_encode_frame (struct t_relay_client *client,
                                           const char *buffer,
                                           unsigned long long length,
                                           unsigned long long *length_frame);

#endif /* __WEECHAT_RELAY_WEBSOCKET_H */
