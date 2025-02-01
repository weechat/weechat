/*
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

#ifndef WEECHAT_PLUGIN_RELAY_REMOTE_NETWORK_H
#define WEECHAT_PLUGIN_RELAY_REMOTE_NETWORK_H

#include <cjson/cJSON.h>

extern int relay_remote_network_send (struct t_relay_remote *remote,
                                      enum t_relay_msg_type msg_type,
                                      const char *data, int data_size);
extern int relay_remote_network_send_json (struct t_relay_remote *remote,
                                           cJSON *json);
extern int relay_remote_network_connect (struct t_relay_remote *remote);
extern void relay_remote_network_disconnect (struct t_relay_remote *remote);

#endif /* WEECHAT_PLUGIN_RELAY_REMOTE_NETWORK_H */
