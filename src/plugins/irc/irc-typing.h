/*
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_TYPING_H
#define WEECHAT_PLUGIN_IRC_TYPING_H

struct t_irc_server;

extern int irc_typing_signal_typing_self_cb (const void *pointer, void *data,
                                             const char *signal,
                                             const char *type_data,
                                             void *signal_data);
extern void irc_typing_send_to_targets (struct t_irc_server *server);
extern void irc_typing_channel_set_nick (struct t_irc_channel *channel,
                                         const char *nick,
                                         int state);
extern void irc_typing_channel_reset (struct t_irc_channel *channel);

#endif /* WEECHAT_PLUGIN_IRC_TYPING_H */
