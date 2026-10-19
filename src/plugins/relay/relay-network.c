/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Network functions for relay plugin */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <gnutls/gnutls.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-network.h"
#include "relay-config.h"


int relay_network_init_ok = 0;
int relay_network_init_tls_cert_key_ok = 0;

gnutls_certificate_credentials_t relay_gnutls_x509_cred;
gnutls_priority_t *relay_gnutls_priority_cache = NULL;
gnutls_dh_params_t *relay_gnutls_dh_params = NULL;


/*
 * Set TLS certificate/key file.
 *
 * If verbose == 1, a message is displayed if successful, otherwise a warning
 * (if no cert/key found in file).
 */

void
relay_network_set_tls_cert_key (int verbose)
{
    const char *ptr_path;
    char *certkey_path;
    int ret;
    struct t_hashtable *options;

    gnutls_certificate_free_credentials (relay_gnutls_x509_cred);
    gnutls_certificate_allocate_credentials (&relay_gnutls_x509_cred);

    relay_network_init_tls_cert_key_ok = 0;

    ptr_path = weechat_config_string (relay_config_network_tls_cert_key);

    if (!ptr_path || !ptr_path[0])
    {
        if (verbose)
        {
            weechat_printf_date_tags (
                NULL, 0, "tls",
                _("%s%s: no TLS certificate/key found (option "
                  "relay.network.tls_cert_key is empty)"),
                weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        }
        return;
    }

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "config");
    certkey_path = weechat_string_eval_path_home (ptr_path,
                                                  NULL, NULL, options);
    weechat_hashtable_free (options);

    if (certkey_path && certkey_path[0])
    {
        if (access (certkey_path, R_OK) == 0)
        {
            ret = gnutls_certificate_set_x509_key_file (relay_gnutls_x509_cred,
                                                        certkey_path,
                                                        certkey_path,
                                                        GNUTLS_X509_FMT_PEM);
            if (ret >= 0)
            {
                relay_network_init_tls_cert_key_ok = 1;
                if (verbose)
                {
                    weechat_printf_date_tags (
                        NULL, 0, "tls",
                        _("%s: TLS certificate and key have been "
                          "set"),
                        RELAY_PLUGIN_NAME);
                }
            }
            else
            {
                if (verbose)
                {
                    weechat_printf_date_tags (
                        NULL, 0, "tls",
                        _("%s%s: gnutls error: %s: %s "
                          "(option relay.network.tls_cert_key)"),
                        weechat_prefix ("error"),
                        RELAY_PLUGIN_NAME,
                        gnutls_strerror_name (ret),
                        gnutls_strerror (ret));
                }
            }
        }
        else
        {
            if (verbose)
            {
                weechat_printf_date_tags (
                    NULL, 0, "tls",
                    _("%s%s: error: file with TLS certificate/key "
                      "is not readable: \"%s\" "
                      "(option relay.network.tls_cert_key)"),
                    weechat_prefix ("error"), RELAY_PLUGIN_NAME,
                    certkey_path);
            }
        }
    }

    free (certkey_path);
}

/*
 * Set gnutls priority cache.
 */

void
relay_network_set_priority (void)
{
    if (gnutls_priority_init (relay_gnutls_priority_cache,
                              weechat_config_string (
                                  relay_config_network_tls_priorities),
                              NULL) != GNUTLS_E_SUCCESS)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to initialize priority for TLS"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        free (relay_gnutls_priority_cache);
        relay_gnutls_priority_cache = NULL;
    }
}

/*
 * Parse an IP address (IPv4 or IPv6).
 *
 * An IPv4-mapped IPv6 address ("::ffff:a.b.c.d") is converted to IPv4.
 *
 * Return:
 *   1: OK (*family and addr are set)
 *   0: error (invalid address)
 */

int
relay_network_parse_ip (const char *address, int *family, unsigned char *addr)
{
    const unsigned char prefix_ipv4_mapped[12] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF,
    };

    if (!address || !address[0] || !family || !addr)
        return 0;

    memset (addr, 0, 16);

    if (inet_pton (AF_INET, address, addr) == 1)
    {
        *family = AF_INET;
        return 1;
    }

    if (inet_pton (AF_INET6, address, addr) == 1)
    {
        if (memcmp (addr, prefix_ipv4_mapped, sizeof (prefix_ipv4_mapped)) == 0)
        {
            memmove (addr, addr + 12, 4);
            memset (addr + 4, 0, 12);
            *family = AF_INET;
        }
        else
        {
            *family = AF_INET6;
        }
        return 1;
    }

    return 0;
}

/*
 * Parse a network in CIDR notation: "address/prefix_length", for example
 * "192.168.1.0/24" or "2001:db8::/32".
 *
 * The prefix length is optional: if not given, the network contains only the
 * address (prefix length is 32 for IPv4 and 128 for IPv6).
 *
 * An IPv4-mapped IPv6 network ("::ffff:a.b.c.d/n", with n >= 96) is converted
 * to IPv4 ("a.b.c.d/(n-96)").
 *
 * Return:
 *   1: OK (cidr is set)
 *   0: error (invalid network)
 */

int
relay_network_parse_cidr (const char *string, struct t_relay_network_cidr *cidr)
{
    const char *pos_slash;
    char *address;
    long prefix_len;
    int rc, family, max_prefix_len;
    unsigned char addr[16];

    if (!string || !string[0] || !cidr)
        return 0;

    rc = 0;

    pos_slash = strchr (string, '/');
    address = (pos_slash) ?
        weechat_strndup (string, pos_slash - string) : strdup (string);
    if (!address)
        goto end;

    /* Parse the address; a v4-mapped IPv6 address is converted to IPv4. */
    if (!relay_network_parse_ip (address, &family, addr))
        goto end;
    max_prefix_len = (family == AF_INET) ? 32 : 128;

    if (pos_slash)
    {
        if (!isdigit ((unsigned char)pos_slash[1])
            || !weechat_util_parse_long (pos_slash + 1, 10, &prefix_len))
        {
            goto end;
        }
        if ((family == AF_INET) && strchr (address, ':'))
        {
            /* IPv4-mapped IPv6 network: prefix is relative to IPv6. */
            if ((prefix_len < 96) || (prefix_len > 128))
                goto end;
            prefix_len -= 96;
        }
        if ((prefix_len < 0) || (prefix_len > max_prefix_len))
            goto end;
    }
    else
    {
        prefix_len = max_prefix_len;
    }

    cidr->family = family;
    memcpy (cidr->addr, addr, sizeof (cidr->addr));
    cidr->prefix_len = (int)prefix_len;
    rc = 1;

end:
    free (address);
    return rc;
}

/*
 * Check if an address (in binary form) belongs to a network.
 *
 * Argument "addr" is 4 bytes for IPv4 and 16 bytes for IPv6.
 *
 * Return:
 *   1: address is in the network
 *   0: address is not in the network
 */

int
relay_network_cidr_match (struct t_relay_network_cidr *cidr,
                          int family, const unsigned char *addr)
{
    int full_bytes, remaining_bits;
    unsigned char mask;

    if (!cidr || !addr || (cidr->family != family))
        return 0;

    full_bytes = cidr->prefix_len / 8;
    remaining_bits = cidr->prefix_len % 8;

    if ((full_bytes > 0) && (memcmp (cidr->addr, addr, full_bytes) != 0))
        return 0;

    if (remaining_bits > 0)
    {
        mask = (unsigned char)(0xFF << (8 - remaining_bits));
        if ((cidr->addr[full_bytes] & mask) != (addr[full_bytes] & mask))
            return 0;
    }

    return 1;
}

/*
 * Check if an IP address (as string) belongs to at least one of the networks.
 *
 * Return:
 *   1: address is in at least one network
 *   0: address is not in any network or is invalid
 */

int
relay_network_address_in_networks (const char *address,
                                   struct t_relay_network_cidr *networks,
                                   int num_networks)
{
    unsigned char addr[16];
    int family, i;

    if (!networks || (num_networks <= 0))
        return 0;

    if (!relay_network_parse_ip (address, &family, addr))
        return 0;

    for (i = 0; i < num_networks; i++)
    {
        if (relay_network_cidr_match (&networks[i], family, addr))
            return 1;
    }

    return 0;
}

/*
 * Initialize network for relay.
 */

void
relay_network_init (void)
{
    relay_network_init_ok = 0;
    relay_network_init_tls_cert_key_ok = 0;

    /* Credentials */
    gnutls_certificate_allocate_credentials (&relay_gnutls_x509_cred);
    relay_network_set_tls_cert_key (0);

    /* Priority */
    relay_gnutls_priority_cache = malloc (sizeof (*relay_gnutls_priority_cache));
    if (relay_gnutls_priority_cache)
        relay_network_set_priority ();

    relay_network_init_ok = 1;
}

/*
 * End network for relay.
 */

void
relay_network_end (void)
{
    if (relay_network_init_ok)
    {
        if (relay_gnutls_priority_cache)
        {
            gnutls_priority_deinit (*relay_gnutls_priority_cache);
            free (relay_gnutls_priority_cache);
            relay_gnutls_priority_cache = NULL;
        }
        if (relay_gnutls_dh_params)
        {
            gnutls_dh_params_deinit (*relay_gnutls_dh_params);
            free (relay_gnutls_dh_params);
            relay_gnutls_dh_params = NULL;
        }
        gnutls_certificate_free_credentials (relay_gnutls_x509_cred);

        relay_network_init_ok = 0;
    }
}
