/*
 * SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
