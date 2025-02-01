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

#ifndef WEECHAT_PLUGIN_RELAY_WEECHAT_PROTOCOL_H
#define WEECHAT_PLUGIN_RELAY_WEECHAT_PROTOCOL_H

#define RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER   (1 << 0)
#define RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST (1 << 1)
#define RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS  (1 << 2)
#define RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE  (1 << 3)

#define RELAY_WEECHAT_PROTOCOL_SYNC_ALL         \
    (RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER |       \
     RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST |     \
     RELAY_WEECHAT_PROTOCOL_SYNC_BUFFERS |      \
     RELAY_WEECHAT_PROTOCOL_SYNC_UPGRADE)

#define RELAY_WEECHAT_PROTOCOL_SYNC_FOR_BUFFER  \
    (RELAY_WEECHAT_PROTOCOL_SYNC_BUFFER |       \
     RELAY_WEECHAT_PROTOCOL_SYNC_NICKLIST)

#define RELAY_WEECHAT_PROTOCOL_CALLBACK(__command)                      \
    int                                                                 \
    relay_weechat_protocol_cb_##__command (                             \
        struct t_relay_client *client,                                  \
        const char *id,                                                 \
        const char *command,                                            \
        int argc,                                                       \
        char **argv,                                                    \
        char **argv_eol)

#define RELAY_WEECHAT_PROTOCOL_MIN_ARGS(__min_args)                     \
    (void) id;                                                          \
    (void) command;                                                     \
    (void) argv;                                                        \
    (void) argv_eol;                                                    \
    if (argc < __min_args)                                              \
    {                                                                   \
        if (weechat_relay_plugin->debug >= 1)                           \
        {                                                               \
            weechat_printf (NULL,                                       \
                            _("%s%s: too few arguments received from "  \
                              "client %s%s%s for command \"%s\" "       \
                              "(received: %d arguments, expected: at "  \
                              "least %d)"),                             \
                            weechat_prefix ("error"),                   \
                            RELAY_PLUGIN_NAME,                          \
                            RELAY_COLOR_CHAT_CLIENT,                    \
                            client->desc,                               \
                            RELAY_COLOR_CHAT,                           \
                            command,                                    \
                            argc,                                       \
                            __min_args);                                \
        }                                                               \
        return WEECHAT_RC_ERROR;                                        \
    }

typedef int (t_relay_weechat_cmd_func)(struct t_relay_client *client,
                                       const char *id, const char *command,
                                       int argc, char **argv, char **argv_eol);

struct t_relay_weechat_protocol_cb
{
    char *name;                        /* relay command                     */
    t_relay_weechat_cmd_func *cmd_function; /* callback                     */
};

extern int relay_weechat_protocol_signal_buffer_cb (const void *pointer,
                                                    void *data,
                                                    const char *signal,
                                                    const char *type_data,
                                                    void *signal_data);
extern int relay_weechat_protocol_hsignal_nicklist_cb (const void *pointer,
                                                       void *data,
                                                       const char *signal,
                                                       struct t_hashtable *hashtable);
extern int relay_weechat_protocol_signal_upgrade_cb (const void *pointer,
                                                     void *data,
                                                     const char *signal,
                                                     const char *type_data,
                                                     void *signal_data);
extern int relay_weechat_protocol_timer_nicklist_cb (const void *pointer,
                                                     void *data,
                                                     int remaining_calls);
extern void relay_weechat_protocol_recv (struct t_relay_client *client,
                                         const char *data);

#endif /* WEECHAT_PLUGIN_RELAY_WEECHAT_PROTOCOL_H */
