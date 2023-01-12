/*
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

#ifndef WEECHAT_PLUGIN_RELAY_H
#define WEECHAT_PLUGIN_RELAY_H

#define weechat_plugin weechat_relay_plugin
#define RELAY_PLUGIN_NAME "relay"
#define RELAY_PLUGIN_PRIORITY 5000

extern struct t_weechat_plugin *weechat_relay_plugin;

extern int relay_signal_upgrade_received;

/* relay protocol */

enum t_relay_protocol
{
    RELAY_PROTOCOL_WEECHAT = 0,        /* WeeChat protocol                  */
    RELAY_PROTOCOL_IRC,                /* IRC protocol (IRC proxy)          */
    /* number of relay protocols */
    RELAY_NUM_PROTOCOLS,
};

#define RELAY_COLOR_CHAT weechat_color("chat")
#define RELAY_COLOR_CHAT_HOST weechat_color("chat_host")
#define RELAY_COLOR_CHAT_BUFFER weechat_color("chat_buffer")
#define RELAY_COLOR_CHAT_CLIENT weechat_color(weechat_config_string(relay_config_color_client))

extern char *relay_protocol_string[];

extern int relay_protocol_search (const char *name);

#endif /* WEECHAT_PLUGIN_RELAY_H */
