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

#ifndef WEECHAT_PLUGIN_IRC_NICK_H
#define WEECHAT_PLUGIN_IRC_NICK_H

#define IRC_NICK_VALID_CHARS_RFC1459 "abcdefghijklmnopqrstuvwxyzABCDEFGHI" \
    "JKLMNOPQRSTUVWXYZ0123456789-[]\\`_^{|}"
#define IRC_NICK_INVALID_CHARS_RFC8265 " ,:\n\r*?.!@"

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
    char *prefix;                   /* current prefix (higher prefix set in  */
                                    /* prefixes); string with just one char  */
    int away;                       /* 1 if nick is away                     */
    char *account;                  /* account name of the user              */
    char *realname;                 /* realname (aka gecos) of the user      */
    char *color;                    /* color for nickname                    */
    struct t_irc_nick *prev_nick;   /* link to previous nick on channel      */
    struct t_irc_nick *next_nick;   /* link to next nick on channel          */
};

extern int irc_nick_valid (struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern int irc_nick_is_nick (struct t_irc_server *server, const char *string);
extern char *irc_nick_find_color (const char *nickname);
extern char *irc_nick_find_color_name (const char *nickname);
extern void irc_nick_set_host (struct t_irc_nick *nick, const char *host);
extern int irc_nick_is_op_or_higher (struct t_irc_server *server,
                                     struct t_irc_nick *nick);
extern int irc_nick_has_prefix_mode (struct t_irc_server *server,
                                     struct t_irc_nick *nick,
                                     char prefix_mode);
extern const char *irc_nick_get_prefix_color_name (struct t_irc_server *server,
                                                   char prefix);
extern void irc_nick_nicklist_set_prefix_color_all (void);
extern void irc_nick_nicklist_set_color_all (void);
extern struct t_irc_nick *irc_nick_new_in_channel (struct t_irc_server *server,
                                                   struct t_irc_channel *channel,
                                                   const char *nickname,
                                                   const char *host,
                                                   const char *prefixes,
                                                   int away,
                                                   const char *account,
                                                   const char *realname);
extern struct t_irc_nick *irc_nick_new (struct t_irc_server *server,
                                        struct t_irc_channel *channel,
                                        const char *nickname,
                                        const char *host,
                                        const char *prefixes,
                                        int away,
                                        const char *account,
                                        const char *realname);
extern void irc_nick_change (struct t_irc_server *server,
                             struct t_irc_channel *channel,
                             struct t_irc_nick *nick, const char *new_nick);
extern void irc_nick_set_mode (struct t_irc_server *server,
                               struct t_irc_channel *channel,
                               struct t_irc_nick *nick, int set, char mode);
extern void irc_nick_realloc_prefixes (struct t_irc_server *server,
                                       int old_length, int new_length);
extern void irc_nick_free (struct t_irc_server *server,
                           struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern void irc_nick_free_all (struct t_irc_server *server,
                               struct t_irc_channel *channel);
extern struct t_irc_nick *irc_nick_search (struct t_irc_server *server,
                                           struct t_irc_channel *channel,
                                           const char *nickname);
extern int *irc_nick_count (struct t_irc_server *server,
                            struct t_irc_channel *channel, int *size);
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
extern const char *irc_nick_color_for_msg (struct t_irc_server *server,
                                           int server_message,
                                           struct t_irc_nick *nick,
                                           const char *nickname);
extern const char * irc_nick_color_for_pv (struct t_irc_channel *channel,
                                           const char *nickname);
extern char *irc_nick_default_ban_mask (struct t_irc_nick *nick);
extern struct t_hdata *irc_nick_hdata_nick_cb (const void *pointer,
                                               void *data,
                                               const char *hdata_name);
extern int irc_nick_add_to_infolist (struct t_infolist *infolist,
                                     struct t_irc_nick *nick);
extern void irc_nick_print_log (struct t_irc_nick *nick);

#endif /* WEECHAT_PLUGIN_IRC_NICK_H */
