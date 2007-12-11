/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

#include "irc-server.h"
#include "irc-channel.h"

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

struct t_irc_nick
{
    char *nick;                     /* nickname                              */
    char *host;                     /* full hostname                         */
    int flags;                      /* chanowner/chanadmin (unrealircd),     */
                                    /* op, halfop, voice, away               */
    char *color;                    /* color for nickname in chat window     */
    struct t_irc_nick *prev_nick;   /* link to previous nick on channel      */
    struct t_irc_nick *next_nick;   /* link to next nick on channel          */
};

extern int irc_nick_find_color (struct t_irc_nick *);
extern void irc_nick_get_gui_infos (struct t_irc_nick *, int *, char *, int *);
extern struct t_irc_nick *irc_nick_new (struct t_irc_server *,
                                        struct t_irc_channel *, char *,
                                        int, int, int, int, int, int, int);
extern void irc_nick_change (struct t_irc_server *, struct t_irc_channel *,
                             struct t_irc_nick *, char *);
extern void irc_nick_free (struct t_irc_channel *, struct t_irc_nick *);
extern void irc_nick_free_all (struct t_irc_channel *);
extern struct t_irc_nick *irc_nick_search (struct t_irc_channel *, char *);
extern void irc_nick_count (struct t_irc_channel *, int *, int *, int *, int *, int *);
extern void irc_nick_set_away (struct t_irc_channel *, struct t_irc_nick *, int);
extern char *irc_nick_as_prefix (struct t_irc_nick *, char *, char *);
extern void irc_nick_print_log (struct t_irc_nick *);

#endif /* irc-nick.h */
