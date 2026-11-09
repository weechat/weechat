/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_ECHO_H
#define WEECHAT_PLUGIN_OTR_ECHO_H

#include <time.h>

/*
 * With the IRCv3 capability echo-message the server sends our own messages
 * back to us. That echo carries the ciphertext. We remember what we sent
 * (cipher line + the plaintext the user typed) and swap them back; echoes
 * of pure protocol traffic (query, AKE, fragments) are hidden.
 */

#define OTR_ECHO_PENDING_MAX 32
#define OTR_ECHO_PENDING_TTL 60

struct t_otr_echo_pending
{
    char *server;                      /* internal name of IRC server       */
    char *peer;                        /* nick of the peer (lower case)     */
    char *cipher;                      /* OTR message sent                  */
    char *plain;                       /* plain text sent                   */
    time_t date;                       /* date of the message               */
};

extern struct t_otr_echo_pending otr_echo_pending_queue[OTR_ECHO_PENDING_MAX];
extern int otr_echo_pending_count;

extern void otr_echo_pending_clear_all (void);
extern void otr_echo_pending_add (const char *server, const char *peer,
                                  const char *cipher, const char *plain);
extern char *otr_echo_replace (const char *server, const char *message,
                               const char *host, const char *target,
                               const char *text);

#endif /* WEECHAT_PLUGIN_OTR_ECHO_H */
