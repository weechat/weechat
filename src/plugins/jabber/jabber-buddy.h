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


#ifndef __WEECHAT_JABBER_BUDDY_H
#define __WEECHAT_JABBER_BUDDY_H 1

#define JABBER_BUDDY_CHANOWNER  1
#define JABBER_BUDDY_CHANADMIN  2
#define JABBER_BUDDY_CHANADMIN2 4
#define JABBER_BUDDY_OP         8
#define JABBER_BUDDY_HALFOP     16
#define JABBER_BUDDY_VOICE      32
#define JABBER_BUDDY_AWAY       64
#define JABBER_BUDDY_CHANUSER   128
#define JABBER_BUDDY_SET_FLAG(buddy, set, flag) \
    if (set) \
        buddy->flags |= flag; \
    else \
        buddy->flags &= 0xFFFF - flag;

#define JABBER_BUDDY_GROUP_OP       "1|op"
#define JABBER_BUDDY_GROUP_HALFOP   "2|halfop"
#define JABBER_BUDDY_GROUP_VOICE    "3|voice"
#define JABBER_BUDDY_GROUP_CHANUSER "4|chanuser"
#define JABBER_BUDDY_GROUP_NORMAL   "5|normal"

struct t_jabber_server;
struct t_jabber_muc;

struct t_jabber_buddy
{
    char *name;                     /* buddyname                             */
    char *host;                     /* full hostname                         */
    int flags;                      /* chanowner/chanadmin, op, halfop,      */
                                    /* voice, away                           */
    const char *color;              /* color for buddyname in chat window    */
    struct t_jabber_buddy *prev_buddy;   /* link to previous buddy in MUC    */
    struct t_jabber_buddy *next_buddy;   /* link to next buddy in MUC        */
};

extern int jabber_buddy_valid (struct t_jabber_server *server,
                               struct t_jabber_muc *muc,
                               struct t_jabber_buddy *buddy);
extern const char *jabber_buddy_find_color (struct t_jabber_buddy *buddy);
extern struct t_jabber_buddy *jabber_buddy_new (struct t_jabber_server *server,
                                                struct t_jabber_muc *muc,
                                                const char *buddy_name,
                                                int is_chanowner,
                                                int is_chanadmin,
                                                int is_chanadmin2,
                                                int is_op,
                                                int is_halfop,
                                                int has_voice,
                                                int is_chanuser,
                                                int is_away);
extern void jabber_buddy_change (struct t_jabber_server *server,
                                 struct t_jabber_muc *muc,
                                 struct t_jabber_buddy *buddy,
                                 const char *new_buddy);
extern void jabber_buddy_set (struct t_jabber_server *server,
                              struct t_jabber_muc *muc,
                              struct t_jabber_buddy *buddy, int set, int flag);
extern void jabber_buddy_free (struct t_jabber_server *server,
                               struct t_jabber_muc *muc,
                               struct t_jabber_buddy *buddy);
extern void jabber_buddy_free_all (struct t_jabber_server *server,
                                   struct t_jabber_muc *muc);
extern struct t_jabber_buddy *jabber_buddy_search (struct t_jabber_server *server,
                                                   struct t_jabber_muc *muc,
                                                   const char *buddyname);
extern void jabber_buddy_count (struct t_jabber_server *server,
                                struct t_jabber_muc *muc, int *total,
                                int *count_op, int *count_halfop,
                                int *count_voice, int *count_normal);
extern void jabber_buddy_set_away (struct t_jabber_server *server,
                                   struct t_jabber_muc *muc,
                                   struct t_jabber_buddy *buddy, int is_away);
extern char *jabber_buddy_as_prefix (struct t_jabber_buddy *buddy,
                                     const char *buddyname,
                                     const char *force_color);
extern int jabber_buddy_add_to_infolist (struct t_infolist *infolist,
                                         struct t_jabber_buddy *buddy);
extern void jabber_buddy_print_log (struct t_jabber_buddy *buddy);

#endif /* jabber-buddy.h */
