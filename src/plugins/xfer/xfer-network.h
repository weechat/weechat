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

#ifndef WEECHAT_PLUGIN_XFER_NETWORK_H
#define WEECHAT_PLUGIN_XFER_NETWORK_H

#include <sys/socket.h>

extern int xfer_network_resolve_addr (const char *str_address,
                                      const char *str_port,
                                      struct sockaddr *addr,
                                      socklen_t *addr_len,
                                      int ai_flags);
extern void xfer_network_write_pipe (struct t_xfer *xfer, int status,
                                     int error);
extern void xfer_network_connect_init (struct t_xfer *xfer);
extern void xfer_network_child_kill (struct t_xfer *xfer);
extern int xfer_network_connect (struct t_xfer *xfer);
extern void xfer_network_accept (struct t_xfer *xfer);

#endif /* WEECHAT_PLUGIN_XFER_NETWORK_H */
