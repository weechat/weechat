/*
 * Copyright (C) 2021-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <time.h>

enum t_typing_status_state
{
    TYPING_STATUS_STATE_OFF = 0,
    TYPING_STATUS_STATE_TYPING,
    TYPING_STATUS_STATE_PAUSED,
    TYPING_STATUS_STATE_CLEARED,
    /* number of typing status statuses */
    TYPING_STATUS_NUM_STATES,
};

/* typing status */

struct t_typing_status
{
    int state;                            /* current state                  */
    time_t last_typed;                    /* when was last char typed       */
};

extern char *typing_status_state_string[];
extern struct t_hashtable *typing_status_self;
extern struct t_hashtable *typing_status_nicks;

extern int typing_status_search_state (const char *state);
extern struct t_typing_status *typing_status_self_add (struct t_gui_buffer *buffer,
                                                       int state,
                                                       int last_typed);
extern struct t_typing_status *typing_status_self_search (struct t_gui_buffer *buffer);
extern struct t_typing_status *typing_status_nick_add (struct t_gui_buffer *buffer,
                                                       const char *nick,
                                                       int state,
                                                       int last_typed);
extern struct t_typing_status *typing_status_nick_search (struct t_gui_buffer *buffer,
                                                          const char *nick);
extern void typing_status_nick_remove (struct t_gui_buffer *buffer,
                                       const char *nick);
extern void typing_status_end (void);

#endif /* WEECHAT_PLUGIN_TYPING_STATUS_H */
