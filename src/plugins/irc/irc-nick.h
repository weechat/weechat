/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_IRC_NICK_H
#define __WEECHAT_IRC_NICK_H 1

#define IRC_NICK_DEFAULT_PREFIXES_LIST "@%+~&!-"

#define IRC_NICK_CHANOWNER  1
#define IRC_NICK_CHANADMIN  2
#define IRC_NICK_OP         4
#define IRC_NICK_HALFOP     8
#define IRC_NICK_VOICE      16
#define IRC_NICK_AWAY       32
#define IRC_NICK_CHANADMIN2 64
#define IRC_NICK_CHANUSER   128
#define IRC_NICK_SET_FLAG(nick, set, flag) \
    if (set) \
        nick->flags |= flag; \
    else \
        nick->flags &= 0xFFFF - flag;

#define IRC_NICK_GROUP_OP       "1|op"
#define IRC_NICK_GROUP_HALFOP   "2|halfop"
#define IRC_NICK_GROUP_VOICE    "3|voice"
#define IRC_NICK_GROUP_CHANUSER "4|chanuser"
#define IRC_NICK_GROUP_NORMAL   "5|normal"

struct t_irc_server;
struct t_irc_channel;

struct t_irc_nick
{
    char *name;                     /* nickname                              */
    char *host;                     /* full hostname                         */
    int flags;                      /* chanowner/chanadmin (unrealircd),     */
                                    /* op, halfop, voice, away               */
    char *color;                    /* color for nickname in chat window     */
    struct t_irc_nick *prev_nick;   /* link to previous nick on channel      */
    struct t_irc_nick *next_nick;   /* link to next nick on channel          */
};

extern struct t_irc_nick *irc_nick_new (struct t_irc_server *server,
                                        struct t_irc_channel *channel,
                                        char *nick_name, int is_chanowner,
                                        int is_chanadmin, int is_chanadmin2,
                                        int is_op, int is_halfop,
                                        int has_voice, int is_chanuser);
extern void irc_nick_change (struct t_irc_server *server,
                             struct t_irc_channel *channel,
                             struct t_irc_nick *nick, char *new_nick);
extern void irc_nick_set (struct t_irc_channel *channel,
                          struct t_irc_nick *nick, int set, int flag);
extern void irc_nick_free (struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern void irc_nick_free_all (struct t_irc_channel *channel);
extern struct t_irc_nick *irc_nick_search (struct t_irc_channel *channel,
                                           char *nickname);
extern void irc_nick_count (struct t_irc_channel *channel, int *total,
                            int *count_op, int *count_halfop, int *count_voice,
                            int *count_normal);
extern void irc_nick_set_away (struct t_irc_channel *channel,
                               struct t_irc_nick *nick, int is_away);
extern char *irc_nick_as_prefix (struct t_irc_nick *nick, char *nickname,
                                 char *force_color);
extern void irc_nick_print_log (struct t_irc_nick *nick);

#endif /* irc-nick.h */
