/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
