/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_MSGBUFFER_H
#define WEECHAT_PLUGIN_IRC_MSGBUFFER_H

enum t_irc_msgbuffer_target
{
    IRC_MSGBUFFER_TARGET_WEECHAT = 0,
    IRC_MSGBUFFER_TARGET_SERVER,
    IRC_MSGBUFFER_TARGET_CURRENT,
    IRC_MSGBUFFER_TARGET_PRIVATE,
    /* number of msgbuffer targets */
    IRC_MSGBUFFER_NUM_TARGETS,
};

struct t_irc_server;

extern struct t_gui_buffer *irc_msgbuffer_get_target_buffer (struct t_irc_server *server,
                                                             const char *nick,
                                                             const char *message,
                                                             const char *alias,
                                                             struct t_gui_buffer *default_buffer);

#endif /* WEECHAT_PLUGIN_IRC_MSGBUFFER_H */
