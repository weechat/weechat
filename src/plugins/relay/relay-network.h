/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_NETWORK_H
#define WEECHAT_PLUGIN_RELAY_NETWORK_H

#include <gnutls/gnutls.h>

extern int relay_network_init_ok;
extern int relay_network_init_tls_cert_key_ok;

extern gnutls_certificate_credentials_t relay_gnutls_x509_cred;
extern gnutls_priority_t *relay_gnutls_priority_cache;
extern gnutls_dh_params_t *relay_gnutls_dh_params;

extern void relay_network_set_tls_cert_key (int verbose);
extern void relay_network_set_priority (void);
extern void relay_network_init (void);
extern void relay_network_end (void);

#endif /* WEECHAT_PLUGIN_RELAY_NETWORK_H */
