/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_MODE_H
#define WEECHAT_PLUGIN_IRC_MODE_H

struct t_irc_server;
struct t_irc_channel;

extern char *irc_mode_get_arguments_colors (const char *arguments);
extern char irc_mode_get_chanmode_type (struct t_irc_server *server,
                                        char chanmode);
extern int irc_mode_channel_set (struct t_irc_server *server,
                                 struct t_irc_channel *channel,
                                 const char *host,
                                 const char *modes,
                                 const char *modes_arguments);
extern void irc_mode_user_set (struct t_irc_server *server, const char *modes,
                               int reset_modes);
extern void irc_mode_registered_mode_change (struct t_irc_server *server);

#endif /* WEECHAT_PLUGIN_IRC_MODE_H */
