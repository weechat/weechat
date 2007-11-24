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
    int color;                      /* color for nickname in chat window     */
    struct t_irc_nick *prev_nick;   /* link to previous nick on channel      */
    struct t_irc_nick *next_nick;   /* link to next nick on channel          */
};

#endif /* irc-nick.h */
