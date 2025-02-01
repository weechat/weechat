/*
 * Copyright (C) 2022-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_JOIN_H
#define WEECHAT_PLUGIN_IRC_JOIN_H

enum t_irc_join_sort
{
    IRC_JOIN_SORT_DISABLED = 0,        /* no sort                           */
    IRC_JOIN_SORT_ALPHA,               /* alphabetic sort                   */
    IRC_JOIN_SORT_BUFFER,              /* sort by buffer number, then alpha */
    /* number of sort methods */
    IRC_JOIN_NUM_SORT,
};

struct t_irc_server;

struct t_irc_join_channel
{
    char *name;                        /* channel name                      */
    char *key;                         /* channel key                       */
};

extern struct t_arraylist *irc_join_split (struct t_irc_server *server,
                                           const char *join,
                                           enum t_irc_join_sort sort);
extern char *irc_join_build_string (struct t_arraylist *arraylist);
extern int irc_join_has_channel (struct t_irc_server *server,
                                 const char *join, const char *channel_name);
extern char *irc_join_add_channel (struct t_irc_server *server,
                                   const char *join, const char *channel_name,
                                   const char *key);
extern char *irc_join_add_channels (struct t_irc_server *server,
                                    const char *join, const char *join2);
extern void irc_join_add_channel_to_autojoin (struct t_irc_server *server,
                                              const char *channel_name,
                                              const char *key);
extern void irc_join_add_channels_to_autojoin (struct t_irc_server *server,
                                               const char *join);
extern char *irc_join_remove_channel (struct t_irc_server *server,
                                      const char *join,
                                      const char *channel_name);
extern void irc_join_remove_channel_from_autojoin (struct t_irc_server *server,
                                                   const char *channel_name);
extern char *irc_join_rename_channel (struct t_irc_server *server,
                                      const char *join,
                                      const char *channel_name,
                                      const char *new_channel_name);
extern void irc_join_rename_channel_in_autojoin (struct t_irc_server *server,
                                                 const char *channel_name,
                                                 const char *new_channel_name);
extern void irc_join_save_channels_to_autojoin (struct t_irc_server *server);
extern char *irc_join_sort_channels (struct t_irc_server *server,
                                     const char *join,
                                     enum t_irc_join_sort sort);
extern void irc_join_sort_autojoin (struct t_irc_server *server,
                                    enum t_irc_join_sort sort);

#endif /* WEECHAT_PLUGIN_IRC_JOIN_H */
