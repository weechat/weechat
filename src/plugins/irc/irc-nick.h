/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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

#define IRC_NICK_VALID_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHI"      \
    "JKLMNOPQRSTUVWXYZ0123456789-[]\\`_^{|}"

#define IRC_NICK_CHANOWNER  1
#define IRC_NICK_CHANADMIN  2
#define IRC_NICK_CHANADMIN2 4
#define IRC_NICK_OP         8
#define IRC_NICK_HALFOP     16
#define IRC_NICK_VOICE      32
#define IRC_NICK_AWAY       64
#define IRC_NICK_CHANUSER   128
#define IRC_NICK_SET_FLAG(nick, set, flag) \
    if (set) \
        nick->flags |= flag; \
    else \
        nick->flags &= 0xFFFF - flag;

#define IRC_NICK_IS_OP(__nick)                                   \
    ((__nick->flags & IRC_NICK_CHANOWNER) ||                     \
     (__nick->flags & IRC_NICK_CHANADMIN) ||                     \
     (__nick->flags & IRC_NICK_CHANADMIN2) ||                    \
     (__nick->flags & IRC_NICK_OP))

#define IRC_NICK_GROUP_CHANOWNER  "01|chanowner"
#define IRC_NICK_GROUP_CHANADMIN  "02|chanadmin"
#define IRC_NICK_GROUP_CHANADMIN2 "03|chanadmin2"
#define IRC_NICK_GROUP_OP         "04|op"
#define IRC_NICK_GROUP_HALFOP     "05|halfop"
#define IRC_NICK_GROUP_VOICE      "06|voice"
#define IRC_NICK_GROUP_CHANUSER   "07|chanuser"
#define IRC_NICK_GROUP_NORMAL     "08|normal"

struct t_irc_server;
struct t_irc_channel;

struct t_irc_nick
{
    char *name;                     /* nickname                              */
    char *host;                     /* full hostname                         */
    int flags;                      /* chanowner/chanadmin (unrealircd),     */
                                    /* op, halfop, voice, away               */
    const char *color;              /* color for nickname in chat window     */
    struct t_irc_nick *prev_nick;   /* link to previous nick on channel      */
    struct t_irc_nick *next_nick;   /* link to next nick on channel          */
};

extern int irc_nick_valid (struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern int irc_nick_is_nick (const char *string);
extern const char *irc_nick_find_color (const char *nickname);
extern void irc_nick_get_gui_infos (struct t_irc_server *server,
                                    struct t_irc_nick *nick,
                                    char *prefix, int *prefix_color,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_nick_group **group);
extern const char *irc_nick_get_prefix_color_name (int prefix_color);
extern struct t_irc_nick *irc_nick_new (struct t_irc_server *server,
                                        struct t_irc_channel *channel,
                                        const char *nickname,
                                        int is_chanowner,
                                        int is_chanadmin,
                                        int is_chanadmin2,
                                        int is_op,
                                        int is_halfop,
                                        int has_voice,
                                        int is_chanuser,
                                        int is_away);
extern void irc_nick_change (struct t_irc_server *server,
                             struct t_irc_channel *channel,
                             struct t_irc_nick *nick, const char *new_nick);
extern void irc_nick_set (struct t_irc_server *server,
                          struct t_irc_channel *channel,
                          struct t_irc_nick *nick, int set, int flag);
extern void irc_nick_free (struct t_irc_server *server,
                           struct t_irc_channel *channel,
                           struct t_irc_nick *nick);
extern void irc_nick_free_all (struct t_irc_server *server,
                               struct t_irc_channel *channel);
extern struct t_irc_nick *irc_nick_search (struct t_irc_channel *channel,
                                           const char *nickname);
extern void irc_nick_count (struct t_irc_channel *channel, int *total,
                            int *count_op, int *count_halfop, int *count_voice,
                            int *count_normal);
extern void irc_nick_set_away (struct t_irc_server *server,
                               struct t_irc_channel *channel,
                               struct t_irc_nick *nick, int is_away);
extern char *irc_nick_as_prefix (struct t_irc_server *server,
                                 struct t_irc_nick *nick,
                                 const char *nickname,
                                 const char *force_color);
extern const char * irc_nick_color_for_pv (struct t_irc_channel *channel,
                                           const char *nickname);
extern int irc_nick_add_to_infolist (struct t_infolist *infolist,
                                     struct t_irc_nick *nick);
extern void irc_nick_print_log (struct t_irc_nick *nick);

#endif /* irc-nick.h */
