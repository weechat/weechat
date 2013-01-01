/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_IRC_NICK_H
#define __WEECHAT_IRC_NICK_H 1

#define IRC_NICK_VALID_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHI"      \
    "JKLMNOPQRSTUVWXYZ0123456789-[]\\`_^{|}"

/* nicklist group for nicks without prefix is "999|..." */
#define IRC_NICK_GROUP_OTHER_NUMBER 999
#define IRC_NICK_GROUP_OTHER_NAME   "..."

struct t_irc_server;
struct t_irc_channel;

struct t_irc_nick
{
    char *name;                     /* nickname                              */
    char *host;                     /* full hostname                         */
    char *prefixes;                 /* string with prefixes enabled for nick */
    char prefix[2];                 /* current prefix (higher prefix set in  */
                                    /* prefixes)                             */
    int away;                       /* 1 if nick is away                     */
    char *color;                    /* color for nickname in chat window     */
    struct t_irc_nick *prev_nick;   /* link to previous nick on channel      */
    struct t_irc_nick *next_nick;   /* link to next nick on channel          */
};

extern int irc_nick_valid (struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern int irc_nick_is_nick (const char *string);
extern int irc_nick_config_colors_cb (void *data, const char *option,
                                      const char *value);
extern const char *irc_nick_find_color (const char *nickname);
extern const char *irc_nick_find_color_name (const char *nickname);
extern int irc_nick_is_op (struct t_irc_server *server,
                           struct t_irc_nick *nick);
extern int irc_nick_has_prefix_mode (struct t_irc_server *server,
                                     struct t_irc_nick *nick,
                                     char prefix_mode);
extern const char *irc_nick_get_prefix_color_name (struct t_irc_server *server,
                                                   struct t_irc_nick *nick);
extern void irc_nick_nicklist_set_prefix_color_all ();
extern void irc_nick_nicklist_set_color_all ();
extern struct t_irc_nick *irc_nick_new (struct t_irc_server *server,
                                        struct t_irc_channel *channel,
                                        const char *nickname,
                                        const char *prefixes,
                                        int away);
extern void irc_nick_change (struct t_irc_server *server,
                             struct t_irc_channel *channel,
                             struct t_irc_nick *nick, const char *new_nick);
extern void irc_nick_set_mode (struct t_irc_server *server,
                               struct t_irc_channel *channel,
                               struct t_irc_nick *nick, int set, char mode);
extern void irc_nick_free (struct t_irc_server *server,
                           struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern void irc_nick_free_all (struct t_irc_server *server,
                               struct t_irc_channel *channel);
extern struct t_irc_nick *irc_nick_search (struct t_irc_server *server,
                                           struct t_irc_channel *channel,
                                           const char *nickname);
extern void irc_nick_count (struct t_irc_server *server,
                            struct t_irc_channel *channel, int *total,
                            int *count_op, int *count_halfop, int *count_voice,
                            int *count_normal);
extern void irc_nick_set_away (struct t_irc_server *server,
                               struct t_irc_channel *channel,
                               struct t_irc_nick *nick, int is_away);
extern const char *irc_nick_mode_for_display (struct t_irc_server *server,
                                              struct t_irc_nick *nick,
                                              int prefix);
extern const char *irc_nick_as_prefix (struct t_irc_server *server,
                                       struct t_irc_nick *nick,
                                       const char *nickname,
                                       const char *force_color);
extern const char *irc_nick_color_for_message (struct t_irc_server *server,
                                               struct t_irc_nick *nick,
                                               const char *nickname);
extern const char *irc_nick_color_for_server_message (struct t_irc_server *server,
                                                      struct t_irc_nick *nick,
                                                      const char *nickname);
extern const char * irc_nick_color_for_pv (struct t_irc_channel *channel,
                                           const char *nickname);
extern struct t_hdata *irc_nick_hdata_nick_cb (void *data,
                                               const char *hdata_name);
extern int irc_nick_add_to_infolist (struct t_infolist *infolist,
                                     struct t_irc_nick *nick);
extern void irc_nick_print_log (struct t_irc_nick *nick);

#endif /* __WEECHAT_IRC_NICK_H */
