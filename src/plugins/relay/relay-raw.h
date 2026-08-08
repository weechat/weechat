/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
