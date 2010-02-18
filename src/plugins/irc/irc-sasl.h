/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_IRC_SASL_H
#define __WEECHAT_IRC_SASL_H 1

/* SASL authentication mechanisms */

enum t_irc_sasl_mechanism
{
    IRC_SASL_MECHANISM_PLAIN = 0,
    IRC_SASL_MECHANISM_DH_BLOWFISH,
    /* number of SASL mechanisms */
    IRC_NUM_SASL_MECHANISMS,
};

extern char *irc_sasl_mechanism_string[];

extern char *irc_sasl_mechanism_plain (const char *sasl_username,
                                       const char *sasl_password);
extern char *irc_sasl_mechanism_dh_blowfish (const char *data_base64,
                                             const char *sasl_username,
                                             const char *sasl_password);

#endif /* irc-sasl.h */
