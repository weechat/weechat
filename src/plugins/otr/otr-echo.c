/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Support of IRCv3 echo-message in OTR plugin */

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-echo.h"
#include "otr-message.h"
#include "otr-util.h"


/* With the IRCv3 capability echo-message the server sends our own messages
 * back to us. That echo carries the ciphertext. We remember what we sent
 * (cipher line + the plaintext the user typed) and swap them back; echoes
 * of pure protocol traffic (query, AKE, fragments) are hidden. */

#define OTR_ECHO_PENDING_MAX 32
#define OTR_ECHO_PENDING_TTL 60

struct t_otr_echo_pending
{
    char *server, *peer, *cipher, *plain;
    time_t t;
};

struct t_otr_echo_pending otr_echo_pending_queue[OTR_ECHO_PENDING_MAX];
int otr_echo_pending_count = 0;

void
otr_echo_pending_drop (int i)
{
    free (otr_echo_pending_queue[i].server);
    free (otr_echo_pending_queue[i].peer);
    free (otr_echo_pending_queue[i].cipher);
    free (otr_echo_pending_queue[i].plain);
    memmove (&otr_echo_pending_queue[i], &otr_echo_pending_queue[i + 1],
             (size_t)(otr_echo_pending_count - i - 1)
             * sizeof (struct t_otr_echo_pending));
    otr_echo_pending_count--;
    memset (&otr_echo_pending_queue[otr_echo_pending_count], 0,
            sizeof (struct t_otr_echo_pending));
}

void
otr_echo_pending_clear_all (void)
{
    while (otr_echo_pending_count > 0)
        otr_echo_pending_drop (0);
}

void
otr_echo_pending_purge (void)
{
    time_t now = time (NULL);
    int i = 0;

    while (i < otr_echo_pending_count)
    {
        if (now - otr_echo_pending_queue[i].t > OTR_ECHO_PENDING_TTL)
            otr_echo_pending_drop (i);
        else
            i++;
    }
}

void
otr_echo_pending_add (const char *server, const char *peer, const char *cipher,
                      const char *plain)
{
    struct t_otr_echo_pending *p;

    otr_echo_pending_purge ();
    if (otr_echo_pending_count == OTR_ECHO_PENDING_MAX)
        otr_echo_pending_drop (0);
    p = &otr_echo_pending_queue[otr_echo_pending_count];
    p->server = otr_util_dupstr (server);
    p->peer = otr_util_lc_dup (peer);
    p->cipher = otr_util_dupstr (cipher);
    p->plain = otr_util_dupstr (plain);
    p->t = time (NULL);
    if (p->server && p->peer && p->cipher && p->plain)
        otr_echo_pending_count++;
    else
    {
        free (p->server);
        free (p->peer);
        free (p->cipher);
        free (p->plain);
        memset (p, 0, sizeof (*p));
    }
}

size_t
otr_echo_trimmed_len (const char *s)
{
    size_t l = strlen (s);

    while (l && (s[l - 1] == ' ' || s[l - 1] == '\t'))
        l--;
    return l;
}

/* returns the plaintext (caller frees) or NULL if nothing matches */
char *
otr_echo_pending_take (const char *server, const char *target,
                       const char *cipher)
{
    int i;
    size_t cl = otr_echo_trimmed_len (cipher);
    char *res;

    otr_echo_pending_purge ();
    for (i = 0; i < otr_echo_pending_count; i++)
    {
        if (strcmp (otr_echo_pending_queue[i].server, server) == 0
            && strcasecmp (otr_echo_pending_queue[i].peer, target) == 0
            && otr_echo_trimmed_len (otr_echo_pending_queue[i].cipher) == cl
            && memcmp (otr_echo_pending_queue[i].cipher, cipher, cl) == 0)
        {
            res = otr_echo_pending_queue[i].plain;
            otr_echo_pending_queue[i].plain = NULL;
            otr_echo_pending_drop (i);
            return res;
        }
    }
    return NULL;
}

/* NULL = leave the echoed line alone; "" = hide it */
char *
otr_echo_replace (const char *server, const char *string, const char *host,
                  const char *target, const char *text)
{
    char *plain, *tags = NULL, *res;
    const char *sp;

    if (strncmp (text, "?OTR", 4) != 0)
        return NULL;
    plain = otr_echo_pending_take (server, target, text);
    if (!plain)
        return strdup ("");
    if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
        tags = strndup (string, (size_t)(sp - string) + 1);
    res = otr_message_build_in_privmsgs (tags ? tags : "", host, target, plain,
                                         "", 1);
    free (tags);
    free (plain);
    return res ? res : strdup ("");
}
