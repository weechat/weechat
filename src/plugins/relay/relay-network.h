/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_RELAY_NETWORK_H
#define __WEECHAT_RELAY_NETWORK_H 1

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

extern int relay_network_init_ok;
extern int relay_network_init_ssl_cert_key_ok;

#ifdef HAVE_GNUTLS
extern gnutls_certificate_credentials_t relay_gnutls_x509_cred;
extern gnutls_priority_t *relay_gnutls_priority_cache;
#endif

extern void relay_network_set_ssl_cert_key (int verbose);
extern void relay_network_init ();
extern void relay_network_end ();

#endif /* __WEECHAT_RELAY_NETWORK_H */
