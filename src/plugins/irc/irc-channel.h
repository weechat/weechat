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


#ifndef __WEECHAT_IRC_CHANNEL_H
#define __WEECHAT_IRC_CHANNEL_H 1

#define IRC_CHANNEL_PREFIX "#&+!"

/* channel types */
#define IRC_CHANNEL_TYPE_UNKNOWN  -1
#define IRC_CHANNEL_TYPE_CHANNEL  0
#define IRC_CHANNEL_TYPE_PRIVATE  1

#define IRC_CHANNEL_NICKS_SPEAKING_LIMIT 32

struct t_irc_server;

struct t_irc_channel
{
    int type;                          /* channel type                      */
    char *name;                        /* name of channel (exemple: "#abc") */
    char *topic;                       /* topic of channel (host for pv)    */
    char *modes;                       /* channel modes                     */
    int limit;                         /* user limit (0 is limit not set)   */
    char *key;                         /* channel key (NULL if no key set)  */
    int nicks_count;                   /* # nicks on channel (0 if pv)      */
    int checking_away;                 /* = 1 if checking away with WHO cmd */
    char *away_message;                /* to display away only once in pv   */
    int cycle;                         /* currently cycling (/part + /join) */
    int display_creation_date;         /* 1 for displaying creation date    */
    int nick_completion_reset;         /* 1 for resetting nick completion   */
                                       /* there was some join/part on chan  */
    struct t_irc_nick *nicks;          /* nicks on the channel              */
    struct t_irc_nick *last_nick;      /* last nick on the channel          */
    struct t_weelist *nicks_speaking;  /* for smart completion              */
    struct t_gui_buffer *buffer;       /* buffer allocated for channel      */
    struct t_irc_channel *prev_channel; /* link to previous channel         */
    struct t_irc_channel *next_channel; /* link to next channel             */
};

extern struct t_irc_channel *irc_channel_new (struct t_irc_server *server,
                                              int channel_type,
                                              char *channel_name,
                                              int switch_to_channel);
extern void irc_channel_free (struct t_irc_server *server,
                              struct t_irc_channel *channel);
extern void irc_channel_free_all (struct t_irc_server *server);
extern struct t_irc_channel *irc_channel_search (struct t_irc_server *server,
                                                 char *channel_name);
extern struct t_irc_channel *irc_channel_search_any (struct t_irc_server *server,
                                                     char *channel_name);
extern struct t_irc_channel *irc_channel_search_any_without_buffer (struct t_irc_server *server,
                                                                    char *channel_name);
extern int irc_channel_is_channel (char *string);
extern void irc_channel_remove_away (struct t_irc_channel *channel);
extern void irc_channel_check_away (struct t_irc_server *server,
                                    struct t_irc_channel *channel, int force);
extern void irc_channel_set_away (struct t_irc_channel *channel, char *nick,
                                  int is_away);
extern int irc_channel_get_notify_level (struct t_irc_server *server,
                                         struct t_irc_channel *channel);
extern void irc_channel_set_notify_level (struct t_irc_server *server,
                                          struct t_irc_channel *channel,
                                          int notify);
extern void irc_channel_add_nick_speaking (struct t_irc_channel *channel,
                                           char *nick);
extern void irc_channel_print_log (struct t_irc_channel *channel);

#endif /* irc-channel.h */
