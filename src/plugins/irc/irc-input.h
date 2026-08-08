/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
