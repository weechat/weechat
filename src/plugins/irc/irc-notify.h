/*
 * Copyright (C) 2010-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_NOTIFY_H
#define WEECHAT_PLUGIN_IRC_NOTIFY_H

struct t_irc_server;

struct t_irc_notify
{
    struct t_irc_server *server;       /* server                            */
    char *nick;                        /* nick                              */
    int check_away;                    /* check away status (with whois)    */
    /* current state of nick */
    int is_on_server;                  /* 1 if nick is currently on server  */
                                       /* 0 if nick is not on server        */
                                       /* -1 for unknown status (check is   */
                                       /* in progress)                      */
                                       /* (using answer of ison command)    */
    char *away_message;                /* current away message, NULL if     */
                                       /* nick is not away (using answer of */
                                       /* whois command)                    */
    /* internal stuff */
    int ison_received;                 /* used when receiving ison answer   */
    struct t_irc_notify *prev_notify;  /* link to previous notify           */
    struct t_irc_notify *next_notify;  /* link to next notify               */
};

extern int irc_notify_valid (struct t_irc_server *server,
                             struct t_irc_notify *notify);
extern struct t_irc_notify *irc_notify_search (struct t_irc_server *server,
                                               const char *nick);
extern void irc_notify_set_server_option (struct t_irc_server *server);
extern struct t_irc_notify *irc_notify_new (struct t_irc_server *server,
                                            const char *nick,
                                            int check_away);
extern void irc_notify_check_now (struct t_irc_notify *notify);
extern void irc_notify_new_for_server (struct t_irc_server *server);
extern void irc_notify_new_for_all_servers ();
extern void irc_notify_free (struct t_irc_server *server,
                             struct t_irc_notify *notify, int remove_monitor);
extern void irc_notify_display_is_on (struct t_irc_server *server,
                                      const char *nick,
                                      const char *host,
                                      struct t_irc_notify *notify,
                                      int is_on_server);
extern void irc_notify_set_is_on_server (struct t_irc_notify *notify,
                                         const char *host, int is_on_server);
extern void irc_notify_free_all (struct t_irc_server *server);
extern void irc_notify_display_list (struct t_irc_server *server);
extern void irc_notify_send_monitor (struct t_irc_server *server);
extern int irc_notify_timer_ison_cb (const void *pointer, void *data,
                                     int remaining_calls);
extern int irc_notify_timer_whois_cb (const void *pointer, void *data,
                                      int remaining_calls);
extern struct t_hdata *irc_notify_hdata_notify_cb (const void *pointer,
                                                   void *data,
                                                   const char *hdata_name);
extern int irc_notify_add_to_infolist (struct t_infolist *infolist,
                                       struct t_irc_notify *notify);
extern void irc_notify_print_log (struct t_irc_server *server);
extern void irc_notify_hook_timer_ison ();
extern void irc_notify_hook_timer_whois ();
extern void irc_notify_init ();
extern void irc_notify_end ();

#endif /* WEECHAT_PLUGIN_IRC_NOTIFY_H */
