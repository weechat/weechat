/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_NETWORK_H
#define WEECHAT_PLUGIN_RELAY_NETWORK_H

#include <gnutls/gnutls.h>

struct t_relay_network_cidr
{
    int family;                        /* AF_INET or AF_INET6               */
    unsigned char addr[16];            /* address (4 bytes used for IPv4)   */
    int prefix_len;                    /* prefix length (in bits)           */
};

extern int relay_network_init_ok;
extern int relay_network_init_tls_cert_key_ok;

extern gnutls_certificate_credentials_t relay_gnutls_x509_cred;
extern gnutls_priority_t *relay_gnutls_priority_cache;
extern gnutls_dh_params_t *relay_gnutls_dh_params;

extern void relay_network_set_tls_cert_key (int verbose);
extern void relay_network_set_priority (void);
extern int relay_network_parse_ip (const char *address, int *family,
                                   unsigned char *addr);
extern int relay_network_parse_cidr (const char *string,
                                     struct t_relay_network_cidr *cidr);
extern int relay_network_cidr_match (struct t_relay_network_cidr *cidr,
                                     int family, const unsigned char *addr);
extern int relay_network_address_in_networks (const char *address,
                                              struct t_relay_network_cidr *networks,
                                              int num_networks);
extern void relay_network_init (void);
extern void relay_network_end (void);

#endif /* WEECHAT_PLUGIN_RELAY_NETWORK_H */
