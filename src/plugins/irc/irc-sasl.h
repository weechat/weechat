/*
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_IRC_SASL_H
#define WEECHAT_IRC_SASL_H 1

struct t_irc_server;

/* SASL authentication mechanisms */

enum t_irc_sasl_mechanism
{
    IRC_SASL_MECHANISM_PLAIN = 0,
    IRC_SASL_MECHANISM_ECDSA_NIST256P_CHALLENGE,
    IRC_SASL_MECHANISM_EXTERNAL,
    IRC_SASL_MECHANISM_DH_BLOWFISH,
    IRC_SASL_MECHANISM_DH_AES,
    /* number of SASL mechanisms */
    IRC_NUM_SASL_MECHANISMS,
};

extern char *irc_sasl_mechanism_string[];

extern char *irc_sasl_mechanism_plain (const char *sasl_username,
                                       const char *sasl_password);
extern char *irc_sasl_mechanism_ecdsa_nist256p_challenge (struct t_irc_server *server,
                                                          const char *data_base64,
                                                          const char *sasl_username,
                                                          const char *sasl_key);
extern char *irc_sasl_mechanism_dh_blowfish (const char *data_base64,
                                             const char *sasl_username,
                                             const char *sasl_password);
extern char *irc_sasl_mechanism_dh_aes (const char *data_base64,
                                        const char *sasl_username,
                                        const char *sasl_password);

#endif /* WEECHAT_IRC_SASL_H */
