/*
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_RELAY_API_PROTOCOL_H
#define WEECHAT_PLUGIN_RELAY_API_PROTOCOL_H

#define RELAY_API_PROTOCOL_CALLBACK(__command)                          \
    int                                                                 \
    relay_api_protocol_cb_##__command (                                 \
        struct t_relay_client *client,                                  \
        struct t_relay_http_request *request)

struct t_relay_api_protocol_ctxt
{
    struct t_relay_client *client;
    struct t_relay_http_request *request;
    char *resource;
};

typedef int (t_relay_api_cmd_func)(struct t_relay_client *client,
                                   struct t_relay_http_request *request);

struct t_relay_api_protocol_cb
{
    char *method;                       /* method (eg: "GET", "POST", etc.) */
    char *resource;                     /* resource (eg: "buffers")         */
    int min_args;                       /* min number of items in path      */
    int max_args;                       /* max number of items in path      */
    t_relay_api_cmd_func *cmd_function; /* callback                         */
};

extern int relay_api_protocol_signal_buffer_cb (const void *pointer,
                                                void *data,
                                                const char *signal,
                                                const char *type_data,
                                                void *signal_data);
extern int relay_api_protocol_hsignal_nicklist_cb (const void *pointer,
                                                   void *data,
                                                   const char *signal,
                                                   struct t_hashtable *hashtable);
extern int relay_api_protocol_signal_upgrade_cb (const void *pointer,
                                                 void *data,
                                                 const char *signal,
                                                 const char *type_data,
                                                 void *signal_data);
extern void relay_api_protocol_recv_http (struct t_relay_client *client,
                                          struct t_relay_http_request *request);
extern void relay_api_protocol_recv_json (struct t_relay_client *client,
                                          const char *json);

#endif /* WEECHAT_PLUGIN_RELAY_API_PROTOCOL_H */
