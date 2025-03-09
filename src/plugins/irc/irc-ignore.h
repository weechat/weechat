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

#ifndef WEECHAT_PLUGIN_IRC_IGNORE_H
#define WEECHAT_PLUGIN_IRC_IGNORE_H

#include <regex.h>

struct t_irc_server;
struct t_irc_channel;

struct t_irc_ignore
{
    int number;                        /* ignore number                     */
    char *mask;                        /* nick / host mask                  */
    regex_t *regex_mask;               /* regex for mask                    */
    char *server;                      /* server name ("*" == any server)   */
    char *channel;                     /* channel name ("*" == any channel) */
    struct t_irc_ignore *prev_ignore;  /* link to previous ignore           */
    struct t_irc_ignore *next_ignore;  /* link to next ignore               */
};

extern struct t_irc_ignore *irc_ignore_list;
extern struct t_irc_ignore *last_irc_ignore;

extern int irc_ignore_valid (struct t_irc_ignore *ignore);
extern struct t_irc_ignore *irc_ignore_search (const char *mask,
                                               const char *server,
                                               const char *channel);
extern struct t_irc_ignore *irc_ignore_search_by_number (int number);
extern struct t_irc_ignore *irc_ignore_new (const char *mask,
                                            const char *server,
                                            const char *channel);
extern int irc_ignore_check_server (struct t_irc_ignore *ignore,
                                    const char *server);
extern int irc_ignore_check_channel (struct t_irc_ignore *ignore,
                                     struct t_irc_server *server,
                                     const char *channel,
                                     const char *nick);
extern int irc_ignore_check_host (struct t_irc_ignore *ignore,
                                  const char *nick, const char *host);
extern int irc_ignore_check (struct t_irc_server *server,
                             const char *channel, const char *nick,
                             const char *host);
extern void irc_ignore_free (struct t_irc_ignore *ignore);
extern void irc_ignore_free_all (void);
extern struct t_hdata *irc_ignore_hdata_ignore_cb (const void *pointer,
                                                   void *data,
                                                   const char *hdata_name);
extern int irc_ignore_add_to_infolist (struct t_infolist *infolist,
                                       struct t_irc_ignore *ignore);
extern void irc_ignore_print_log (void);

#endif /* WEECHAT_PLUGIN_IRC_IGNORE_H */
