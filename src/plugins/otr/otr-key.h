/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_KEY_H
#define WEECHAT_PLUGIN_OTR_KEY_H

/* Max duration of the key generation (in milliseconds). */

#define OTR_KEY_GENERATION_TIMEOUT (5 * 60 * 1000)

/* Private key being generated in background (child process). */

struct t_otr_key_generation
{
    char *account;                     /* OTR account ("nick@server")       */
    char *protocol;                    /* protocol (OTR_PROTOCOL)           */
    char **output;                     /* output of child process (key)     */
    char **error;                      /* errors of child process           */
    char **peers;                      /* peers waiting for the key         */
                                       /* (separated by "\n")               */
    struct t_hook *hook;               /* process generating the key        */
    struct t_otr_key_generation *prev_generation; /* link to previous       */
    struct t_otr_key_generation *next_generation; /* link to next           */
};

extern struct t_otr_key_generation *otr_key_generations;
extern struct t_otr_key_generation *last_otr_key_generation;

extern int otr_key_write_fingerprints (void);
extern int otr_key_generate (const char *account, const char *protocol,
                             const char *peer);
extern struct t_otr_key_generation *otr_key_generation_search (
    const char *account, const char *protocol);
extern int otr_key_generation_remove_peer (
    struct t_otr_key_generation *generation, const char *peer);
extern void otr_key_generation_free_all (void);

#endif /* WEECHAT_PLUGIN_OTR_KEY_H */
