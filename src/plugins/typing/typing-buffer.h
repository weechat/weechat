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

#ifndef WEECHAT_PLUGIN_TYPING_BUFFER_H
#define WEECHAT_PLUGIN_TYPING_BUFFER_H

#include <stdio.h>
#include <time.h>

struct t_infolist;

enum t_typing_buffer_status
{
    TYPING_BUFFER_STATUS_OFF = 0,
    TYPING_BUFFER_STATUS_TYPING,
    TYPING_BUFFER_STATUS_PAUSED,
    TYPING_BUFFER_STATUS_CLEARED,
    /* number of typing buffer statuses */
    TYPING_BUFFER_NUM_STATUSES,
};

/* own typing status */

struct t_typing_buffer
{
    struct t_gui_buffer *buffer;          /* pointer to buffer              */
    int status;                           /* status                         */
    time_t last_typed;                    /* last char typed                */
    time_t last_signal_sent;              /* last signal sent               */
    struct t_typing_buffer *prev_buffer;  /* link to previous buffer        */
    struct t_typing_buffer *next_buffer;  /* link to next buffer            */
};

extern struct t_typing_buffer *typing_buffers;
extern struct t_typing_buffer *last_typing_buffer;

extern int typing_buffer_valid (struct t_typing_buffer *typing_buffer);
extern struct t_typing_buffer *typing_buffer_add (struct t_gui_buffer *buffer);
extern struct t_typing_buffer *typing_buffer_search_buffer (struct t_gui_buffer *buffer);
extern void typing_buffer_free (struct t_typing_buffer *typing_buffer);
extern int typing_buffer_add_to_infolist (struct t_infolist *infolist,
                                          struct t_typing_buffer *typing_buffer);

#endif /* WEECHAT_PLUGIN_TYPING_BUFFER_H */
