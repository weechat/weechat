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


#ifndef __WEECHAT_NETWORK_H
#define __WEECHAT_NETWORK_H 1

struct t_hook;

extern void network_init ();
extern void network_end ();
extern int network_pass_proxy (int sock, const char *address, int port);
extern int network_connect_to (int sock, unsigned long address, int port);
extern void network_connect_with_fork (struct t_hook *);

#endif /* wee-network.h */
