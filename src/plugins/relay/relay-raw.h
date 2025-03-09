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

#ifndef WEECHAT_PLUGIN_RELAY_RAW_H
#define WEECHAT_PLUGIN_RELAY_RAW_H

#include <time.h>

#include "relay-client.h"

#define RELAY_RAW_BUFFER_NAME "relay_raw"
#define RELAY_RAW_PREFIX_RECV "-->"
#define RELAY_RAW_PREFIX_SEND "<--"

#define RELAY_RAW_FLAG_RECV   (1 << 0)
#define RELAY_RAW_FLAG_SEND   (1 << 1)
#define RELAY_RAW_FLAG_BINARY (1 << 2)

struct t_relay_remote;

struct t_relay_raw_message
{
    time_t date;                       /* date/time of message              */
    int date_usec;                     /* microseconds of date              */
    char *prefix;                      /* prefix                            */
    char *message;                     /* message                           */
    struct t_relay_raw_message *prev_message; /* pointer to prev. message   */
    struct t_relay_raw_message *next_message; /* pointer to next message    */
};

struct t_relay_client;

extern struct t_gui_buffer *relay_raw_buffer;
extern int irc_relay_messages_count;
extern struct t_relay_raw_message *relay_raw_messages, *last_relay_raw_message;

extern void relay_raw_open (int switch_to_buffer);
extern struct t_relay_raw_message *relay_raw_message_add_to_list (time_t date,
                                                                  int date_usec,
                                                                  const char *prefix,
                                                                  const char *message);
extern void relay_raw_print_client (struct t_relay_client *client,
                                    enum t_relay_msg_type msg_type, int flags,
                                    const char *data, int data_size);
extern void relay_raw_print_remote (struct t_relay_remote *remote,
                                    enum t_relay_msg_type msg_type, int flags,
                                    const char *data, int data_size);
extern void relay_raw_message_free_all (void);
extern int relay_raw_add_to_infolist (struct t_infolist *infolist,
                                      struct t_relay_raw_message *raw_message);

#endif /* WEECHAT_PLUGIN_RELAY_RAW_H */
