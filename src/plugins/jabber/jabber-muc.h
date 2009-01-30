/*
 * Copyright (c) 2009 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_JABBER_MUC_H
#define __WEECHAT_JABBER_MUC_H 1

/* MUC types */
#define JABBER_MUC_TYPE_UNKNOWN  -1
#define JABBER_MUC_TYPE_MUC      0
#define JABBER_MUC_TYPE_PRIVATE  1

#define JABBER_MUC_BUDDIES_SPEAKING_LIMIT 128

struct t_jabber_server;

struct t_jabber_muc_speaking
{
    char *buddy;                       /* buddy speaking                    */
    time_t time_last_message;          /* time                              */
    struct t_jabber_muc_speaking *prev_buddy; /* pointer to previous buddy  */
    struct t_jabber_muc_speaking *next_buddy; /* pointer to next buddy      */
};

struct t_jabber_muc
{
    int type;                          /* MUC type                          */
    char *name;                        /* name of MUC (exemple: "test")     */
    char *topic;                       /* topic of MUC (host for pv)        */
    char *modes;                       /* MUC modes                         */
    int limit;                         /* user limit (0 is limit not set)   */
    char *key;                         /* MUC key (NULL if no key set)      */
    char *away_message;                /* to display away only once in pv   */
    int nick_completion_reset;         /* 1 for resetting nick completion   */
                                       /* there was some join/part on chan  */
    int buddies_count;                 /* # buddies in MUC (0 if pv)        */
    struct t_jabber_buddy *buddies;    /* buddies in MUC                    */
    struct t_jabber_buddy *last_buddy; /* last buddy in MUC                 */
    struct t_weelist *buddies_speaking[2]; /* for smart completion: first   */
                                       /* list is buddy speaking, second is */
                                       /* speaking to me (highlight)        */
    struct t_jabber_muc_speaking *buddies_speaking_time; /* for smart filter*/
                                       /* of join/quit messages             */
    struct t_jabber_muc_speaking *last_buddy_speaking_time;
    struct t_gui_buffer *buffer;       /* buffer allocated for MUC          */
    char *buffer_as_string;            /* used to return buffer info        */
    struct t_jabber_muc *prev_muc;     /* link to previous MUC              */
    struct t_jabber_muc *next_muc;     /* link to next MUC                  */
};

extern int jabber_muc_valid (struct t_jabber_server *server,
                             struct t_jabber_muc *muc);
extern struct t_jabber_muc *jabber_muc_new (struct t_jabber_server *server,
                                            int muc_type,
                                            const char *muc_name,
                                            int switch_to_muc,
                                            int auto_switch);
extern void jabber_muc_set_topic (struct t_jabber_muc *muc,
                                  const char *topic);
extern void jabber_muc_free (struct t_jabber_server *server,
                             struct t_jabber_muc *muc);
extern void jabber_muc_free_all (struct t_jabber_server *server);
extern struct t_jabber_muc *jabber_muc_search (struct t_jabber_server *server,
                                               const char *muc_name);
extern int jabber_muc_is_muc (const char *string);
extern void jabber_muc_remove_away (struct t_jabber_muc *muc);
extern void jabber_muc_check_away (struct t_jabber_server *server,
                                   struct t_jabber_muc *muc, int force);
extern void jabber_muc_set_away (struct t_jabber_muc *muc,
                                 const char *buddy_name,
                                 int is_away);
extern void jabber_muc_buddy_speaking_add (struct t_jabber_muc *muc,
                                           const char *buddy_name,
                                           int highlight);
extern void jabber_muc_buddy_speaking_rename (struct t_jabber_muc *muc,
                                              const char *old_buddy,
                                              const char *new_buddy);
extern struct t_jabber_muc_speaking *jabber_muc_buddy_speaking_time_search (struct t_jabber_muc *muc,
                                                                            const char *buddy_name,
                                                                            int check_time);
extern void jabber_muc_buddy_speaking_time_remove_old (struct t_jabber_muc *muc);
extern void jabber_muc_buddy_speaking_time_add (struct t_jabber_muc *muc,
                                                const char *buddy_name,
                                                time_t time_last_message);
extern void jabber_muc_buddy_speaking_time_rename (struct t_jabber_muc *muc,
                                                   const char *old_buddy,
                                                   const char *new_buddy);
extern int jabber_muc_add_to_infolist (struct t_infolist *infolist,
                                       struct t_jabber_muc *muc);
extern void jabber_muc_print_log (struct t_jabber_muc *muc);

#endif /* jabber-muc.h */
