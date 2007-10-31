/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* weechat-trigger-libirc.c: Tiny libirc for trigger plugin */

#ifndef WEECHAT_TRIGGER_LIBIRC__H
#define WEECHAT_TRIGGER_LIBIRC__H 1

typedef struct irc_msg
{
    char *userhost;
    char *nick;
    char *user;
    char *host;
    char *command;
    char *channel;
    char *message;
    char *data;
} irc_msg;

irc_msg *irc_parse_msg (char *);
void irc_msg_free (irc_msg *);

#endif /* WEECHAT_TRIGGER_LIBIRC__H */
