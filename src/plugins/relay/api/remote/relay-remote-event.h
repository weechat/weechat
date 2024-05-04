/*
 * Copyright (C) 2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_RELAY_REMOTE_EVENT_H
#define WEECHAT_PLUGIN_RELAY_REMOTE_EVENT_H

#define RELAY_REMOTE_EVENT_CALLBACK(__body_type)                        \
    int                                                                 \
    relay_remote_event_cb_##__body_type (                               \
        struct t_relay_remote_event *event)

struct t_relay_remote_event
{
    struct t_relay_remote *remote;     /* relay remote                      */
    const char *name;                  /* event name (signal, hsignal)      */
    struct t_gui_buffer *buffer;       /* buffer (can be NULL)              */
    cJSON *json;                       /* JSON object                       */
};

typedef int (t_relay_remote_event_func)(struct t_relay_remote_event *event);

struct t_relay_remote_event_cb
{
    char *event_mask;                   /* event name (mask)                */
    t_relay_remote_event_func *func;    /* callback (can be NULL)           */
};

extern void relay_remote_event_recv (struct t_relay_remote *remote,
                                     const char *data);

#endif /* WEECHAT_PLUGIN_RELAY_REMOTE_EVENT_H */
