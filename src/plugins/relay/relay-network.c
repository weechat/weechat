/*
 * relay-network.c - network functions for relay plugin
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>

#include <gnutls/gnutls.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-network.h"
#include "relay-config.h"


int relay_network_init_ok = 0;
int relay_network_init_ssl_cert_key_ok = 0;

gnutls_certificate_credentials_t relay_gnutls_x509_cred;
gnutls_priority_t *relay_gnutls_priority_cache = NULL;
gnutls_dh_params_t *relay_gnutls_dh_params = NULL;


/*
 * Sets SSL certificate/key file.
 *
 * If verbose == 1, a message is displayed if successful, otherwise a warning
 * (if no cert/key found in file).
 */

void
relay_network_set_ssl_cert_key (int verbose)
{
    char *certkey_path;
    int ret;
    struct t_hashtable *options;

    gnutls_certificate_free_credentials (relay_gnutls_x509_cred);
    gnutls_certificate_allocate_credentials (&relay_gnutls_x509_cred);

    relay_network_init_ssl_cert_key_ok = 0;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "config");
    certkey_path = weechat_string_eval_path_home (
        weechat_config_string (relay_config_network_ssl_cert_key),
        NULL, NULL, options);
    if (options)
        weechat_hashtable_free (options);
    if (certkey_path)
    {
        ret = gnutls_certificate_set_x509_key_file (relay_gnutls_x509_cred,
                                                    certkey_path,
                                                    certkey_path,
                                                    GNUTLS_X509_FMT_PEM);
        if (ret >= 0)
        {
            relay_network_init_ssl_cert_key_ok = 1;
            if (verbose)
            {
                weechat_printf (NULL,
                                _("%s: SSL certificate and key have been "
                                  "set"),
                                RELAY_PLUGIN_NAME);
            }
        }
        else
        {
            if (verbose)
            {
                weechat_printf (NULL,
                                _("%s%s: warning: no SSL certificate/key "
                                  "found (option relay.network.ssl_cert_key)"),
                                weechat_prefix ("error"), RELAY_PLUGIN_NAME);
            }
        }
        free (certkey_path);
    }
}

/*
 * Sets gnutls priority cache.
 */

void
relay_network_set_priority ()
{
    if (gnutls_priority_init (relay_gnutls_priority_cache,
                              weechat_config_string (
                                  relay_config_network_ssl_priorities),
                              NULL) != GNUTLS_E_SUCCESS)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to initialize priority for SSL"),
                        weechat_prefix ("error"), RELAY_PLUGIN_NAME);
        free (relay_gnutls_priority_cache);
        relay_gnutls_priority_cache = NULL;
    }
}

/*
 * Initializes network for relay.
 */

void
relay_network_init ()
{
    /* credentials */
    gnutls_certificate_allocate_credentials (&relay_gnutls_x509_cred);
    relay_network_set_ssl_cert_key (0);

    /* priority */
    relay_gnutls_priority_cache = malloc (sizeof (*relay_gnutls_priority_cache));
    if (relay_gnutls_priority_cache)
        relay_network_set_priority ();

    relay_network_init_ok = 1;
}

/*
 * Ends network for relay.
 */

void
relay_network_end ()
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
