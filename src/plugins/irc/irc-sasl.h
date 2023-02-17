/*
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

#ifndef WEECHAT_PLUGIN_IRC_SASL_H
#define WEECHAT_PLUGIN_IRC_SASL_H

#define IRC_SASL_SCRAM_CLIENT_KEY "Client Key"
#define IRC_SASL_SCRAM_SERVER_KEY "Server Key"

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

extern char *irc_sasl_mechanism_string[];

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

#endif /* WEECHAT_PLUGIN_IRC_SASL_H */
