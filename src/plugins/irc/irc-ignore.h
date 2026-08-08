/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
