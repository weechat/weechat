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

#ifndef WEECHAT_PLUGIN_RELAY_WEECHAT_NICKLIST_H
#define WEECHAT_PLUGIN_RELAY_WEECHAT_NICKLIST_H

#define RELAY_WEECHAT_NICKLIST_DIFF_UNKNOWN ' '
#define RELAY_WEECHAT_NICKLIST_DIFF_PARENT  '^'
#define RELAY_WEECHAT_NICKLIST_DIFF_ADDED   '+'
#define RELAY_WEECHAT_NICKLIST_DIFF_REMOVED '-'
#define RELAY_WEECHAT_NICKLIST_DIFF_CHANGED '*'

struct t_relay_weechat_nicklist_item
{
    void *pointer;                     /* pointer on group/nick             */
    char diff;                         /* type of diff (see constants above)*/
    char group;                        /* 1=group, 0=nick                   */
    char visible;                      /* 1=visible, 0=not visible          */
    int level;                         /* level                             */
    char *name;                        /* name of group/nick                */
    char *color;                       /* color for name                    */
    char *prefix;                      /* prefix                            */
    char *prefix_color;                /* color for prefix                  */
};

struct t_relay_weechat_nicklist
{
    int nicklist_count;                /* number of nicks in nicklist       */
                                       /* before receiving first diff       */
    int items_count;                   /* number of nicklist items          */
    struct t_relay_weechat_nicklist_item *items; /* nicklist items          */
};

extern struct t_relay_weechat_nicklist *relay_weechat_nicklist_new (void);
extern void relay_weechat_nicklist_add_item (struct t_relay_weechat_nicklist *nicklist,
                                             char diff,
                                             struct t_gui_nick_group *group,
                                             struct t_gui_nick *nick);
extern void relay_weechat_nicklist_free (struct t_relay_weechat_nicklist *nicklist);

#endif /* WEECHAT_PLUGIN_RELAY_WEECHAT_NICKLIST_H */
