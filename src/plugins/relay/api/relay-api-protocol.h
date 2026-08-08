/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_API_PROTOCOL_H
#define WEECHAT_PLUGIN_RELAY_API_PROTOCOL_H

#define RELAY_API_ALLOWED_METHODS "GET, POST, PUT, DELETE"

#define RELAY_API_CB(__command) &relay_api_protocol_cb_##__command
#define RELAY_API_PROTOCOL_CALLBACK(__command)                          \
    enum t_relay_api_protocol_rc                                        \
    relay_api_protocol_cb_##__command (struct t_relay_client *client)

enum t_relay_api_protocol_rc
{
    RELAY_API_PROTOCOL_RC_OK = 0,      /* callback OK                       */
    RELAY_API_PROTOCOL_RC_BAD_REQUEST, /* bad request (parameters/body)     */
    RELAY_API_PROTOCOL_RC_MEMORY,      /* out of memory                     */
    /* number of data types */
    RELAY_NUM_API_PROTOCOL_RC,
};

typedef enum t_relay_api_protocol_rc (t_relay_api_cmd_func)(struct t_relay_client *client);

struct t_relay_api_protocol_cb
{
    char *method;                       /* method (eg: "GET", "POST", etc.) */
    char *resource;                     /* resource (eg: "buffers")         */
    int auth_required;                  /* authentication required?         */
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
extern int relay_api_protocol_signal_input_cb (const void *pointer, void *data,
                                               const char *signal,
                                               const char *type_data,
                                               void *signal_data);
extern int relay_api_protocol_signal_upgrade_cb (const void *pointer,
                                                 void *data,
                                                 const char *signal,
                                                 const char *type_data,
                                                 void *signal_data);
extern void relay_api_protocol_recv_json (struct t_relay_client *client,
                                          const char *json);
extern void relay_api_protocol_recv_http (struct t_relay_client *client);

#endif /* WEECHAT_PLUGIN_RELAY_API_PROTOCOL_H */
