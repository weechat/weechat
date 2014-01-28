/*
 * Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_TRIGGER_CALLBACK_H
#define __WEECHAT_TRIGGER_CALLBACK_H 1

extern int trigger_callback_signal_cb (void *data, const char *signal,
                                       const char *type_data, void *signal_data);
extern int trigger_callback_hsignal_cb (void *data, const char *signal,
                                        struct t_hashtable *hashtable);
extern char *trigger_callback_modifier_cb (void *data, const char *modifier,
                                           const char *modifier_data,
                                           const char *string);
extern int trigger_callback_print_cb  (void *data, struct t_gui_buffer *buffer,
                                       time_t date, int tags_count,
                                       const char **tags, int displayed,
                                       int highlight, const char *prefix,
                                       const char *message);
extern int trigger_callback_timer_cb  (void *data, int remaining_calls);
extern void trigger_callback_init ();
extern void trigger_callback_end ();

#endif /* __WEECHAT_TRIGGER_CALLBACK_H */
