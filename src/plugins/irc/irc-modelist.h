/*
 * Copyright (C) 2015 Simmo Saan <simmo.saan@gmail.com>
 * Copyright (C) 2018-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_MODELIST_H
#define WEECHAT_PLUGIN_IRC_MODELIST_H

#include <time.h>

/* modelist states */
#define IRC_MODELIST_STATE_EMPTY      0
#define IRC_MODELIST_STATE_RECEIVING  1
#define IRC_MODELIST_STATE_RECEIVED   2
#define IRC_MODELIST_STATE_MODIFIED   3

struct t_irc_server;

struct t_irc_modelist_item
{
    int number;                            /* item number                   */
    char *mask;                            /* modelist mask                 */
    char *setter;                          /* hostmask of setter (optional) */
    time_t datetime;                       /* datetime of setting (optional)*/

    struct t_irc_modelist_item *prev_item; /* pointer to previous item      */
    struct t_irc_modelist_item *next_item; /* pointer to next item          */
};

struct t_irc_modelist
{
    char type;                             /* mode list channel A type      */
    int state;                             /* state                         */

    struct t_irc_modelist_item *items;     /* items in modelist             */
    struct t_irc_modelist_item *last_item; /* last item in modelist         */

    struct t_irc_modelist *prev_modelist;  /* pointer to previous modelist  */
    struct t_irc_modelist *next_modelist;  /* pointer to next modelist      */
};

extern int irc_modelist_valid (struct t_irc_channel *channel,
                               struct t_irc_modelist *modelist);
extern struct t_irc_modelist *irc_modelist_search (struct t_irc_channel *channel,
                                                   char type);
extern struct t_irc_modelist *irc_modelist_new (struct t_irc_channel *channel,
                                                char type);
extern void irc_modelist_free (struct t_irc_channel *channel,
                               struct t_irc_modelist *modelist);
extern void irc_modelist_free_all (struct t_irc_channel *channel);

extern int irc_modelist_item_valid (struct t_irc_modelist *modelist,
                                    struct t_irc_modelist_item *item);
extern struct t_irc_modelist_item *irc_modelist_item_search_mask (struct t_irc_modelist *modelist,
                                                                  const char *mask);
extern struct t_irc_modelist_item *irc_modelist_item_search_number (struct t_irc_modelist *modelist,
                                                                    int number);
extern struct t_irc_modelist_item *irc_modelist_item_new (struct t_irc_modelist *modelist,
                                                          const char *mask,
                                                          const char *setter,
                                                          time_t datetime);
extern void irc_modelist_item_free (struct t_irc_modelist *modelist,
                                    struct t_irc_modelist_item *item);
extern void irc_modelist_item_free_all (struct t_irc_modelist *modelist);

extern struct t_hdata *irc_modelist_hdata_item_cb (const void *pointer,
                                                   void *data,
                                                   const char *hdata_name);
extern struct t_hdata *irc_modelist_hdata_modelist_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern int irc_modelist_item_add_to_infolist (struct t_infolist *infolist,
                                              struct t_irc_modelist_item *item);
extern int irc_modelist_add_to_infolist (struct t_infolist *infolist,
                                         struct t_irc_modelist *modelist);
extern void irc_modelist_item_print_log (struct t_irc_modelist_item *item);
extern void irc_modelist_print_log (struct t_irc_modelist *modelist);

#endif /* WEECHAT_PLUGIN_IRC_MODELIST_H */
