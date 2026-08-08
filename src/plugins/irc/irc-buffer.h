/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_BUFFER_H
#define WEECHAT_PLUGIN_IRC_BUFFER_H

#define IRC_BUFFER_GET_SERVER(__buffer)                                 \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    buffer_plugin = weechat_buffer_get_pointer (__buffer, "plugin");    \
    if (buffer_plugin == weechat_irc_plugin)                            \
        irc_buffer_get_server_and_channel (__buffer, &ptr_server, NULL);

#define IRC_BUFFER_GET_SERVER_CHANNEL(__buffer)                         \
    struct t_weechat_plugin *buffer_plugin = NULL;                      \
    struct t_irc_server *ptr_server = NULL;                             \
    struct t_irc_channel *ptr_channel = NULL;                           \
    buffer_plugin = weechat_buffer_get_pointer (__buffer, "plugin");    \
    if (buffer_plugin == weechat_irc_plugin)                            \
    {                                                                   \
        irc_buffer_get_server_and_channel (__buffer, &ptr_server,       \
                                           &ptr_channel);               \
    }


struct t_gui_buffer;
struct t_irc_server;
struct t_irc_channel;

extern void irc_buffer_get_server_and_channel (struct t_gui_buffer *buffer,
                                               struct t_irc_server **server,
                                               struct t_irc_channel **channel);
extern char *irc_buffer_build_name (const char *server, const char *channel);
extern int irc_buffer_close_cb (const void *pointer, void *data,
                                struct t_gui_buffer *buffer);
extern int irc_buffer_nickcmp_cb (const void *pointer, void *data,
                                  struct t_gui_buffer *buffer,
                                  const char *nick1, const char *nick2);
extern struct t_gui_buffer *irc_buffer_search_server_lowest_number (void);
extern struct t_gui_buffer *irc_buffer_search_private_lowest_number (struct t_irc_server *server);
extern void irc_buffer_move_near_server (struct t_irc_server *server,
                                         int list_buffer, int channel_type,
                                         struct t_gui_buffer *buffer);

#endif /* WEECHAT_PLUGIN_IRC_BUFFER_H */
