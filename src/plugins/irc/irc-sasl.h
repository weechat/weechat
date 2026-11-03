/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_SASL_H
#define WEECHAT_PLUGIN_IRC_SASL_H

#define IRC_SASL_SCRAM_CLIENT_KEY "Client Key"
#define IRC_SASL_SCRAM_SERVER_KEY "Server Key"

/*
 * Iteration count accepted for SCRAM: minimum is the one recommended by
 * RFC 7677, maximum prevents the server from blocking WeeChat for a long time
 */
#define IRC_SASL_SCRAM_MIN_ITERATIONS 4096
#define IRC_SASL_SCRAM_MAX_ITERATIONS 1000000

struct t_irc_server;

/* SASL authentication mechanisms */

enum t_irc_sasl_mechanism
{
    IRC_SASL_MECHANISM_PLAIN = 0,
    IRC_SASL_MECHANISM_SCRAM_SHA_1,
    IRC_SASL_MECHANISM_SCRAM_SHA_256,
    IRC_SASL_MECHANISM_SCRAM_SHA_512,
    IRC_SASL_MECHANISM_ECDSA_NIST256P_CHALLENGE,
    IRC_SASL_MECHANISM_EXTERNAL,
    /* number of SASL mechanisms */
    IRC_NUM_SASL_MECHANISMS,
};

/* status of SCRAM authentication */

enum t_irc_sasl_scram_status
{
    IRC_SASL_SCRAM_STATUS_NONE = 0,    /* no SCRAM authentication          */
    IRC_SASL_SCRAM_STATUS_PENDING,     /* server not authenticated yet     */
    IRC_SASL_SCRAM_STATUS_VERIFIED,    /* server signature verified        */
};

extern char *irc_sasl_mechanism_string[];

extern int irc_sasl_mechanism_is_scram (int sasl_mechanism);
extern char *irc_sasl_mechanism_plain (const char *sasl_username,
                                       const char *sasl_password);
extern char *irc_sasl_mechanism_scram (struct t_irc_server *server,
                                       const char *hash_algo,
                                       const char *data_base64,
                                       const char *sasl_username,
                                       const char *sasl_password,
                                       char **sasl_error);
extern char *irc_sasl_mechanism_ecdsa_nist256p_challenge (struct t_irc_server *server,
                                                          const char *data_base64,
                                                          const char *sasl_username,
                                                          const char *sasl_key,
                                                          char **sasl_error);
extern char *irc_sasl_mechanism_external (const char *sasl_username);

#endif /* WEECHAT_PLUGIN_IRC_SASL_H */
