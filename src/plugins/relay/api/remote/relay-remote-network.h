/*
 * SPDX-FileCopyrightText: 2024-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
