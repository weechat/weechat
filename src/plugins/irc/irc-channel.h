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

#ifndef __WEECHAT_IRC_CHANNEL_H
#define __WEECHAT_IRC_CHANNEL_H 1

#define IRC_CHANNEL_DEFAULT_CHANTYPES "#&+!"

/* channel types */
#define IRC_CHANNEL_TYPE_UNKNOWN  -1
#define IRC_CHANNEL_TYPE_CHANNEL  0
#define IRC_CHANNEL_TYPE_PRIVATE  1

#define IRC_CHANNEL_NICKS_SPEAKING_LIMIT 128

struct t_irc_server;

struct t_irc_channel_speaking
{
    char *nick;                        /* nick speaking                     */
    time_t time_last_message;          /* time                              */
    struct t_irc_channel_speaking *prev_nick; /* pointer to previous nick   */
    struct t_irc_channel_speaking *next_nick; /* pointer to next nick       */
};

struct t_irc_channel
{
    int type;                          /* channel type                      */
    char *name;                        /* name of channel (exemple: "#abc") */
    char *topic;                       /* topic of channel (host for pv)    */
    char *modes;                       /* channel modes                     */
    int limit;                         /* user limit (0 is limit not set)   */
    char *key;                         /* channel key (NULL if no key set)  */
    int names_received;                /* names received (message 366)      */
    int checking_away;                 /* = 1 if checking away with WHO cmd */
    char *away_message;                /* to display away only once in pv   */
    int has_quit_server;               /* =1 if nick has quit (pv only), to */
                                       /* display message when he's back    */
    int cycle;                         /* currently cycling (/part + /join) */
    int part;                          /* /part done on channel?            */
    int nick_completion_reset;         /* 1 for resetting nick completion   */
                                       /* there was some join/part on chan  */
    char *pv_remote_nick_color;        /* color for remote nick in pv       */
    struct t_hook *hook_autorejoin;    /* this time+delay = autorejoin time */
    int nicks_count;                   /* # nicks on channel (0 if pv)      */
    struct t_irc_nick *nicks;          /* nicks on the channel              */
    struct t_irc_nick *last_nick;      /* last nick on the channel          */
    struct t_weelist *nicks_speaking[2]; /* for smart completion: first     */
                                       /* list is nick speaking, second is  */
                                       /* speaking to me (highlight)        */
    struct t_irc_channel_speaking *nicks_speaking_time; /* for smart filter */
                                       /* of join/part/quit messages        */
    struct t_irc_channel_speaking *last_nick_speaking_time;
    struct t_gui_buffer *buffer;       /* buffer allocated for channel      */
    char *buffer_as_string;            /* used to return buffer info        */
    struct t_irc_channel *prev_channel; /* link to previous channel         */
    struct t_irc_channel *next_channel; /* link to next channel             */
};

extern int irc_channel_valid (struct t_irc_server *server,
                              struct t_irc_channel *channel);
extern struct t_irc_channel *irc_channel_new (struct t_irc_server *server,
                                              int channel_type,
                                              const char *channel_name,
                                              int switch_to_channel,
                                              int auto_switch);
extern void irc_channel_set_topic (struct t_irc_channel *channel,
                                   const char *topic);
extern void irc_channel_set_modes (struct t_irc_channel *channel,
                                   const char *modes);
extern void irc_channel_free (struct t_irc_server *server,
                              struct t_irc_channel *channel);
extern void irc_channel_free_all (struct t_irc_server *server);
extern struct t_irc_channel *irc_channel_search (struct t_irc_server *server,
                                                 const char *channel_name);
extern int irc_channel_is_channel (struct t_irc_server *server,
                                   const char *string);
extern void irc_channel_remove_away (struct t_irc_server *server,
                                     struct t_irc_channel *channel);
extern void irc_channel_check_away (struct t_irc_server *server,
                                    struct t_irc_channel *channel);
extern void irc_channel_set_away (struct t_irc_server *server,
                                  struct t_irc_channel *channel,
                                  const char *nick_name,
                                  int is_away);
extern void irc_channel_nick_speaking_add (struct t_irc_channel *channel,
                                           const char *nick_name,
                                           int highlight);
extern void irc_channel_nick_speaking_rename (struct t_irc_channel *channel,
                                              const char *old_nick,
                                              const char *new_nick);
extern struct t_irc_channel_speaking *irc_channel_nick_speaking_time_search (struct t_irc_server *server,
                                                                             struct t_irc_channel *channel,
                                                                             const char *nick_name,
                                                                             int check_time);
extern void irc_channel_nick_speaking_time_remove_old (struct t_irc_channel *channel);
extern void irc_channel_nick_speaking_time_add (struct t_irc_server *server,
                                                struct t_irc_channel *channel,
                                                const char *nick_name,
                                                time_t time_last_message);
extern void irc_channel_nick_speaking_time_rename (struct t_irc_server *server,
                                                   struct t_irc_channel *channel,
                                                   const char *old_nick,
                                                   const char *new_nick);
extern void irc_channel_rejoin (struct t_irc_server *server,
                                struct t_irc_channel *channel);
extern int irc_channel_autorejoin_cb (void *data, int remaining_calls);
extern void irc_channel_display_nick_back_in_pv (struct t_irc_server *server,
                                                 struct t_irc_nick *nick,
                                                 const char *nickname);
extern struct t_hdata *irc_channel_hdata_channel_cb (void *data,
                                                     const char *hdata_name);
extern struct t_hdata *irc_channel_hdata_channel_speaking_cb (void *data,
                                                              const char *hdata_name);
extern int irc_channel_add_to_infolist (struct t_infolist *infolist,
                                        struct t_irc_channel *channel);
extern void irc_channel_print_log (struct t_irc_channel *channel);

#endif /* __WEECHAT_IRC_CHANNEL_H */
