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


#ifndef __WEECHAT_IRC_CHANNEL_H
#define __WEECHAT_IRC_CHANNEL_H 1

#define IRC_CHANNEL_PREFIX "#&+!"

/* channel types */
#define IRC_CHANNEL_TYPE_UNKNOWN  -1
#define IRC_CHANNEL_TYPE_CHANNEL  0
#define IRC_CHANNEL_TYPE_PRIVATE  1
#define IRC_CHANNEL_TYPE_DCC_CHAT 2

#define IRC_CHANNEL_NICKS_SPEAKING_LIMIT 32

struct t_irc_channel
{
    int type;                       /* channel type                          */
    void *dcc_chat;                 /* DCC CHAT pointer (NULL if not DCC)    */
    char *name;                     /* name of channel (exemple: "#abc")     */
    char *topic;                    /* topic of channel (host for private)   */
    char *modes;                    /* channel modes                         */
    int limit;                      /* user limit (0 is limit not set)       */
    char *key;                      /* channel key (NULL if no key is set)   */
    int nicks_count;                /* # nicks on channel (0 if dcc/pv)      */
    int checking_away;              /* = 1 if checking away with WHO cmd     */
    char *away_message;             /* to display away only once in private  */
    int cycle;                      /* currently cycling (/part then /join)  */
    int close;                      /* close request (/buffer close)         */
    int display_creation_date;      /* 1 if creation date should be displayed*/
    int nick_completion_reset;      /* 1 if nick completion should be rebuilt*/
                                    /* there was some join/part on channel   */
    struct t_irc_nick *nicks;             /* nicks on the channel            */
    struct t_irc_nick *last_nick;         /* last nick on the channel        */
    struct t_weelist *nicks_speaking;     /* for smart completion            */
    struct t_weelist *last_nick_speaking; /* last nick speaking              */
    struct t_gui_buffer *buffer;          /* buffer allocated for channel    */
    struct t_irc_channel *prev_channel;   /* link to previous channel        */
    struct t_irc_channel *next_channel;   /* link to next channel            */
};

#endif /* irc-channel.h */
