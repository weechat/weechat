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


#ifndef __WEECHAT_RELAY_CLIENT_H
#define __WEECHAT_RELAY_CLIENT_H 1

/* relay status */

enum t_relay_status
{
    RELAY_STATUS_CONNECTING = 0,       /* connecting to client              */
    RELAY_STATUS_WAITING_AUTH,         /* waiting AUTH from client          */
    RELAY_STATUS_CONNECTED,            /* connected to client               */
    RELAY_STATUS_AUTH_FAILED,          /* AUTH failed with client           */
    RELAY_STATUS_DISCONNECTED,         /* disconnected from client          */
    /* number of relay status */
    RELAY_NUM_STATUS,
};

/* macros for status */

#define RELAY_CLIENT_HAS_ENDED(status) ((status == RELAY_STATUS_AUTH_FAILED) ||      \
                                        (status == RELAY_STATUS_DISCONNECTED))

/* relay client */

struct t_relay_client
{
    int sock;                          /* socket for connection             */
    char *address;                     /* string with IP address            */
    enum t_relay_status status;        /* status (connecting, active,..)    */
    time_t start_time;                 /* time of client connection         */
    struct t_hook *hook_fd;            /* hook for socket or child pipe     */
    struct t_hook *hook_timer;         /* timeout for recever accept        */
    time_t last_activity;              /* time of last byte received/sent   */
    unsigned long bytes_recv;          /* bytes received from client        */
    unsigned long bytes_sent;          /* bytes sent to client              */
    struct t_relay_client *prev_client;/* link to previous client           */
    struct t_relay_client *next_client;/* link to next client               */
};

extern char *relay_client_status_string[];
extern struct t_relay_client *relay_clients;
extern struct t_relay_client *last_relay_client;
extern int relay_client_count;

extern int relay_client_valid (struct t_relay_client *client);
extern struct t_relay_client *relay_client_search_by_number (int number);
extern struct t_relay_client *relay_client_new (int sock, char *address);
extern void relay_client_set_status (struct t_relay_client *client,
                                     enum t_relay_status status);
extern void relay_client_free (struct t_relay_client *client);
extern void relay_client_disconnect (struct t_relay_client *client);
extern void relay_client_disconnect_all ();
extern int relay_client_add_to_infolist (struct t_infolist *infolist,
                                         struct t_relay_client *client);
extern void relay_client_print_log ();

#endif /* relay-client.h */
