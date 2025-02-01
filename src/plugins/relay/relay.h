/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

extern struct t_hdata *relay_hdata_buffer;
extern struct t_hdata *relay_hdata_key;
extern struct t_hdata *relay_hdata_lines;
extern struct t_hdata *relay_hdata_line;
extern struct t_hdata *relay_hdata_line_data;
extern struct t_hdata *relay_hdata_nick_group;
extern struct t_hdata *relay_hdata_nick;
extern struct t_hdata *relay_hdata_completion;
extern struct t_hdata *relay_hdata_completion_word;
extern struct t_hdata *relay_hdata_hotlist;

/* relay protocol */

enum t_relay_protocol
{
    RELAY_PROTOCOL_WEECHAT = 0,        /* WeeChat protocol                  */
    RELAY_PROTOCOL_IRC,                /* IRC protocol (IRC proxy)          */
    RELAY_PROTOCOL_API,                /* HTTP REST API                     */
    /* number of relay protocols */
    RELAY_NUM_PROTOCOLS,
};

/* client/remote status */

enum t_relay_status
{
    RELAY_STATUS_CONNECTING = 0,       /* network connection in progress    */
    RELAY_STATUS_AUTHENTICATING,       /* authentication in progress        */
    RELAY_STATUS_CONNECTED,            /* connected and authenticated       */
    RELAY_STATUS_AUTH_FAILED,          /* authentication failed             */
    RELAY_STATUS_DISCONNECTED,         /* disconnected                      */
    /* number of relay status */
    RELAY_NUM_STATUS,
};

/* type of message exchanged with the peer (client/remote) */

enum t_relay_msg_type
{
    RELAY_MSG_STANDARD,
    RELAY_MSG_PING,
    RELAY_MSG_PONG,
    RELAY_MSG_CLOSE,
    /* number of message types */
    RELAY_NUM_MSG_TYPES,
};

#define RELAY_STATUS_HAS_ENDED(status)                                  \
    ((status == RELAY_STATUS_AUTH_FAILED) ||                            \
     (status == RELAY_STATUS_DISCONNECTED))

#define RELAY_COLOR_CHAT weechat_color("chat")
#define RELAY_COLOR_CHAT_HOST weechat_color("chat_host")
#define RELAY_COLOR_CHAT_BUFFER weechat_color("chat_buffer")
#define RELAY_COLOR_CHAT_CLIENT weechat_color(weechat_config_string(relay_config_color_client))

extern char *relay_protocol_string[];
extern char *relay_status_string[];
extern char *relay_status_name[];
extern char *relay_msg_type_string[];

extern int relay_protocol_search (const char *name);
extern int relay_status_search (const char *name);

#endif /* WEECHAT_PLUGIN_RELAY_H */
