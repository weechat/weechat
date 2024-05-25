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

#ifndef WEECHAT_PLUGIN_RELAY_API_H
#define WEECHAT_PLUGIN_RELAY_API_H

struct t_relay_client;
enum t_relay_status;

#define RELAY_API_VERSION_MAJOR 0
#define RELAY_API_VERSION_MINOR 1
#define RELAY_API_VERSION_PATCH 0
#define RELAY_API_VERSION_NUMBER                                        \
    ((RELAY_API_VERSION_MAJOR << 16)                                    \
     + (RELAY_API_VERSION_MINOR << 8)                                   \
     + RELAY_API_VERSION_PATCH)
#define RELAY_API_VERSION_STR                                           \
    (TO_STR(RELAY_API_VERSION_MAJOR)                                    \
     "."                                                                \
     TO_STR(RELAY_API_VERSION_MINOR)                                    \
     "."                                                                \
     TO_STR(RELAY_API_VERSION_PATCH))

#define RELAY_API_DATA(client, var)                                     \
    (((struct t_relay_api_data *)client->protocol_data)->var)

#define RELAY_API_HTTP_0_EVENT 0, "Event"

enum t_relay_api_colors
{
    RELAY_API_COLORS_ANSI = 0,         /* convert colors to ANSI colors     */
    RELAY_API_COLORS_WEECHAT,          /* keep WeeChat internal color codes */
    RELAY_API_COLORS_STRIP,            /* strip colors                      */
    /* number of data types */
    RELAY_API_NUM_COLORS,
};

struct t_relay_api_data
{
    struct t_hook *hook_signal_buffer;    /* hook for signals "buffer_*"    */
    struct t_hook *hook_hsignal_nicklist; /* hook for hsignals "nicklist_*" */
    struct t_hook *hook_signal_input;     /* hook for signal                */
                                          /* "input_text_changed"           */
    struct t_hook *hook_signal_upgrade;   /* hook for signals "upgrade*"    */
    struct t_hashtable *buffers_closing;  /* ptr -> "id" of buffers closing */
    int sync_enabled;                     /* 1 if sync is enabled           */
    int sync_nicks;                       /* 1 if nicks are synchronized    */
    int sync_input;                       /* 1 if input is synchronized     */
                                          /* (WeeChat -> client)            */
    enum t_relay_api_colors sync_colors;  /* colors to send with sync       */
};

extern long long relay_api_get_buffer_id (struct t_gui_buffer *buffer);
extern enum t_relay_api_colors relay_api_search_colors (const char *colors);
extern void relay_api_hook_signals (struct t_relay_client *client);
extern void relay_api_unhook_signals (struct t_relay_client *client);
extern void relay_api_recv_http (struct t_relay_client *client);
extern void relay_api_recv_json (struct t_relay_client *client,
                                 const char *json);
extern void relay_api_close_connection (struct t_relay_client *client);
extern void relay_api_alloc (struct t_relay_client *client);
extern void relay_api_alloc_with_infolist (struct t_relay_client *client,
                                           struct t_infolist *infolist);
extern enum t_relay_status relay_api_get_initial_status (struct t_relay_client *client);
extern void relay_api_free (struct t_relay_client *client);
extern int relay_api_add_to_infolist (struct t_infolist_item *item,
                                      struct t_relay_client *client,
                                      int force_disconnected_state);
extern void relay_api_print_log (struct t_relay_client *client);

#endif /* WEECHAT_PLUGIN_RELAY_API_H */
