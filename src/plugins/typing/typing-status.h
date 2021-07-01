/*
 * Copyright (C) 2021 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_TYPING_STATUS_H
#define WEECHAT_PLUGIN_TYPING_STATUS_H

#include <stdio.h>
#include <time.h>

struct t_infolist;

enum t_typing_status_status
{
    TYPING_STATUS_STATUS_OFF = 0,
    TYPING_STATUS_STATUS_TYPING,
    TYPING_STATUS_STATUS_PAUSED,
    TYPING_STATUS_STATUS_CLEARED,
    /* number of typing status statuses */
    TYPING_STATUS_NUM_STATUSES,
};

/* self typing status */

struct t_typing_status
{
    int status;                           /* status                         */
    time_t last_typed;                    /* last char typed                */
    time_t last_signal_sent;              /* last signal sent               */
};

extern struct t_hashtable *typing_status_self;

extern struct t_typing_status *typing_status_add (struct t_gui_buffer *buffer);
extern void typing_status_end ();

#endif /* WEECHAT_PLUGIN_TYPING_STATUS_H */
