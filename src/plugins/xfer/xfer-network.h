/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_XFER_NETWORK_H
#define __WEECHAT_XFER_NETWORK_H 1

extern void xfer_network_write_pipe (struct t_xfer *xfer, int status,
                                     int error);
extern void xfer_network_connect_init (struct t_xfer *xfer);
extern void xfer_network_child_kill (struct t_xfer *xfer);
extern int xfer_network_connect (struct t_xfer *xfer);
extern void xfer_network_accept (struct t_xfer *xfer);

#endif /* xfer-network.h */
