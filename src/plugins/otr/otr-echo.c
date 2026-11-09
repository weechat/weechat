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

#define PENDING_MAX 32
#define PENDING_TTL 60

struct pending
{
    char *server, *peer, *cipher, *plain;
    time_t t;
};

static struct pending pending_q[PENDING_MAX];
static int pending_n = 0;

static void
pending_drop (int i)
{
    free (pending_q[i].server);
    free (pending_q[i].peer);
    free (pending_q[i].cipher);
    free (pending_q[i].plain);
    memmove (&pending_q[i], &pending_q[i + 1],
             (size_t)(pending_n - i - 1) * sizeof (struct pending));
    pending_n--;
    memset (&pending_q[pending_n], 0, sizeof (struct pending));
}

void
pending_clear_all (void)
{
    while (pending_n > 0)
        pending_drop (0);
}

static void
pending_purge (void)
{
    time_t now = time (NULL);
    int i = 0;

    while (i < pending_n)
    {
        if (now - pending_q[i].t > PENDING_TTL)
            pending_drop (i);
        else
            i++;
    }
}

void
pending_add (const char *server, const char *peer, const char *cipher,
             const char *plain)
{
    struct pending *p;

    pending_purge ();
    if (pending_n == PENDING_MAX)
        pending_drop (0);
    p = &pending_q[pending_n];
    p->server = dupstr (server);
    p->peer = lc_dup (peer);
    p->cipher = dupstr (cipher);
    p->plain = dupstr (plain);
    p->t = time (NULL);
    if (p->server && p->peer && p->cipher && p->plain)
        pending_n++;
    else
    {
        free (p->server);
        free (p->peer);
        free (p->cipher);
        free (p->plain);
        memset (p, 0, sizeof (*p));
    }
}

static size_t
trimmed_len (const char *s)
{
    size_t l = strlen (s);

    while (l && (s[l - 1] == ' ' || s[l - 1] == '\t'))
        l--;
    return l;
}

/* returns the plaintext (caller frees) or NULL if nothing matches */
static char *
pending_take (const char *server, const char *target, const char *cipher)
{
    int i;
    size_t cl = trimmed_len (cipher);
    char *res;

    pending_purge ();
    for (i = 0; i < pending_n; i++)
    {
        if (strcmp (pending_q[i].server, server) == 0
            && strcasecmp (pending_q[i].peer, target) == 0
            && trimmed_len (pending_q[i].cipher) == cl
            && memcmp (pending_q[i].cipher, cipher, cl) == 0)
        {
            res = pending_q[i].plain;
            pending_q[i].plain = NULL;
            pending_drop (i);
            return res;
        }
    }
    return NULL;
}

/* NULL = leave the echoed line alone; "" = hide it */
char *
echo_replace (const char *server, const char *string, const char *host,
              const char *target, const char *text)
{
    char *plain, *tags = NULL, *res;
    const char *sp;

    if (strncmp (text, "?OTR", 4) != 0)
        return NULL;
    plain = pending_take (server, target, text);
    if (!plain)
        return strdup ("");
    if (string[0] == '@' && (sp = strchr (string, ' ')) != NULL)
        tags = strndup (string, (size_t)(sp - string) + 1);
    res = build_in_privmsgs (tags ? tags : "", host, target, plain, "", 1);
    free (tags);
    free (plain);
    return res ? res : strdup ("");
}
