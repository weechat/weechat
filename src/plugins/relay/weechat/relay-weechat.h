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

#ifndef WEECHAT_PLUGIN_RELAY_WEECHAT_H
#define WEECHAT_PLUGIN_RELAY_WEECHAT_H

struct t_relay_client;
enum t_relay_status;

#define RELAY_WEECHAT_DATA(client, var)                          \
    (((struct t_relay_weechat_data *)client->protocol_data)->var)

#define RELAY_WEECHAT_AUTH_OK(client)                            \
    ((RELAY_WEECHAT_DATA(client, password_ok)                    \
      && RELAY_WEECHAT_DATA(client, totp_ok)))

enum t_relay_weechat_compression
{
    RELAY_WEECHAT_COMPRESSION_OFF = 0, /* no compression of binary objects  */
    RELAY_WEECHAT_COMPRESSION_ZLIB,    /* zlib compression                  */
#ifdef HAVE_ZSTD
    RELAY_WEECHAT_COMPRESSION_ZSTD,    /* Zstandard compression             */
#endif
    /* number of compressions */
    RELAY_WEECHAT_NUM_COMPRESSIONS,
};

struct t_relay_weechat_data
{
    /* handshake status */
    int handshake_done;                /* 1 if handshake has been done      */

    /* handshake options */
    enum t_relay_weechat_compression compression; /* compression type       */
    int escape_commands;               /* 1 if backslashes are interpreted  */
                                       /* in commands sent by client        */

    /* authentication status (init command) */
    int password_ok;                   /* password received and OK?         */
    int totp_ok;                       /* TOTP received and OK?             */

    /* sync of buffers */
    struct t_hashtable *buffers_sync;  /* buffers synchronized (events      */
                                       /* received for these buffers)       */
    struct t_hook *hook_signal_buffer;    /* hook for signals "buffer_*"    */
    struct t_hook *hook_hsignal_nicklist; /* hook for hsignals "nicklist_*" */
    struct t_hook *hook_signal_upgrade;   /* hook for signals "upgrade*"    */
    struct t_hashtable *buffers_nicklist; /* send nicklist for these buffers*/
    struct t_hook *hook_timer_nicklist;   /* timer for sending nicklist     */
};

extern char *relay_weechat_compression_string[];

extern int relay_weechat_compression_search (const char *compression);
extern void relay_weechat_hook_signals (struct t_relay_client *client);
extern void relay_weechat_unhook_signals (struct t_relay_client *client);
extern void relay_weechat_hook_timer_nicklist (struct t_relay_client *client);
extern void relay_weechat_recv (struct t_relay_client *client,
                                const char *data);
extern void relay_weechat_close_connection (struct t_relay_client *client);
extern void relay_weechat_alloc (struct t_relay_client *client);
extern void relay_weechat_alloc_with_infolist (struct t_relay_client *client,
                                               struct t_infolist *infolist);
extern enum t_relay_status relay_weechat_get_initial_status (struct t_relay_client *client);
extern void relay_weechat_free (struct t_relay_client *client);
extern int relay_weechat_add_to_infolist (struct t_infolist_item *item,
                                          struct t_relay_client *client,
                                          int force_disconnected_state);
extern void relay_weechat_print_log (struct t_relay_client *client);

#endif /* WEECHAT_PLUGIN_RELAY_WEECHAT_H */
