/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_MESSAGE_H
#define WEECHAT_PLUGIN_IRC_MESSAGE_H

struct t_irc_message_split_context
{
    struct t_hashtable *hashtable;     /* hashtable with msgs/args/count    */
    int number;                        /* current msg index (starts to 1)   */
    long total_bytes;                  /* total bytes of messages split     */
                                       /* (+ 1 byte between each message)   */
};

struct t_irc_server;
struct t_irc_channel;

extern void irc_message_parse_params (const char *parameters,
                                      char ***params, int *num_params);
extern void irc_message_parse (struct t_irc_server *server, const char *message,
                               char **tags, char **message_without_tags,
                               char **nick, char **user, char **host,
                               char **command, char **channel,
                               char **arguments, char **text,
                               char ***params, int *num_params,
                               int *pos_command, int *pos_arguments,
                               int *pos_channel, int *pos_text);
extern struct t_hashtable *irc_message_parse_to_hashtable (struct t_irc_server *server,
                                                           const char *message);
extern struct t_hashtable *irc_message_parse_cap_value (const char *value);
extern void irc_message_parse_cap_multiline_value (struct t_irc_server *server,
                                                   const char *value);
extern int irc_message_is_empty (const char *message);
extern char *irc_message_convert_charset (const char *message,
                                          int pos_start,
                                          const char *modifier,
                                          const char *modifier_data);
extern const char *irc_message_get_nick_from_host (const char *host);
extern const char *irc_message_get_address_from_host (const char *host);
extern int irc_message_ignored (struct t_irc_server *server,
                                const char *message);
extern char *irc_message_replace_vars (struct t_irc_server *server,
                                       const char *channel_name,
                                       const char *string);
extern char *irc_message_hide_password (struct t_irc_server *server,
                                        const char *target, const char *text);
extern struct t_hashtable *irc_message_split (struct t_irc_server *server,
                                              const char *message);

#endif /* WEECHAT_PLUGIN_IRC_MESSAGE_H */
