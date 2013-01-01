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

#ifndef __WEECHAT_RELAY_IRC_H
#define __WEECHAT_RELAY_IRC_H 1

struct t_relay_client;

#define RELAY_IRC_DATA(client, var)                              \
    (((struct t_relay_irc_data *)client->protocol_data)->var)

struct t_relay_irc_data
{
    char *address;                     /* client address (used when sending */
                                       /* data to client)                   */
    int password_ok;                   /* password received and ok?         */
    char *nick;                        /* nick for client                   */
    int user_received;                 /* command "USER" received           */
    int connected;                     /* 1 if client is connected as IRC   */
                                       /* client                            */
    int server_capabilities;           /* server capabilities enabled (one  */
                                       /* bit per capability)               */
    struct t_hook *hook_timer_signals_joins;/* timer to hooks signals and   */
                                            /* send joins to client         */
    struct t_hook *hook_signal_irc_in2;     /* signal "irc_in2"             */
    struct t_hook *hook_signal_irc_outtags; /* signal "irc_outtags"         */
    struct t_hook *hook_signal_irc_disc;    /* signal "irc_disconnected"    */
    struct t_hook *hook_hsignal_irc_redir;  /* hsignal "irc_redirection_..."*/
};

enum t_relay_irc_command
{
    RELAY_IRC_CMD_JOIN = 0,
    RELAY_IRC_CMD_PART,
    RELAY_IRC_CMD_QUIT,
    RELAY_IRC_CMD_NICK,
    RELAY_IRC_CMD_PRIVMSG,
    /* number of relay irc commands */
    RELAY_IRC_NUM_CMD,
};

enum t_relay_irc_server_capab
{
    RELAY_IRC_CAPAB_SERVER_TIME = 0,
    /* number of server capabilities */
    RELAY_IRC_NUM_CAPAB,
};

extern void relay_irc_recv (struct t_relay_client *client,
                                   const char *data);
extern void relay_irc_close_connection (struct t_relay_client *client);
extern void relay_irc_alloc (struct t_relay_client *client);
extern void relay_irc_alloc_with_infolist (struct t_relay_client *client,
                                           struct t_infolist *infolist);
extern void relay_irc_free (struct t_relay_client *client);
extern int relay_irc_add_to_infolist (struct t_infolist_item *item,
                                      struct t_relay_client *client);
extern void relay_irc_print_log (struct t_relay_client *client);

#endif /* __WEECHAT_RELAY_IRC_H */
