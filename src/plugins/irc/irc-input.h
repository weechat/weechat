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

#ifndef WEECHAT_PLUGIN_IRC_INPUT_H
#define WEECHAT_PLUGIN_IRC_INPUT_H

#include <time.h>

struct t_hashtable;
struct t_gui_buffer;

extern void irc_input_user_message_display (struct t_irc_server *server,
                                            time_t date,
                                            int date_usec,
                                            struct t_hashtable *tags,
                                            const char *target,
                                            const char *address,
                                            const char *command,
                                            const char *ctcp_type,
                                            const char *text,
                                            int decode_colors);
extern int irc_input_data_cb (const void *pointer, void *data,
                              struct t_gui_buffer *buffer,
                              const char *input_data);
extern int irc_input_send_cb (const void *pointer, void *data,
                              const char *signal, const char *type_data,
                              void *signal_data);

#endif /* WEECHAT_PLUGIN_IRC_INPUT_H */
