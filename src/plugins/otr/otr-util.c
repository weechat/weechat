/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Utility functions for OTR plugin */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-util.h"


char *
otr_util_strf (const char *fmt, ...)
{
    va_list ap, ap2;
    int n;
    char *s;

    va_start (ap, fmt);
    va_copy (ap2, ap);
    n = vsnprintf (NULL, 0, fmt, ap);
    va_end (ap);
    if (n < 0)
    {
        va_end (ap2);
        return NULL;
    }
    s = malloc ((size_t)n + 1);
    if (s)
        vsnprintf (s, (size_t)n + 1, fmt, ap2);
    va_end (ap2);
    return s;
}

char *
otr_util_dupstr (const char *s)
{
    return strdup (s ? s : "");
}

char *
otr_util_lc_dup (const char *s)
{
    char *r = otr_util_dupstr (s), *p;

    if (!r)
        return NULL;
    for (p = r; *p; p++)
        *p = (char)tolower ((unsigned char)*p);
    return r;
}

void
otr_util_sb_init (struct t_otr_sbuf *b)
{
    b->s = NULL;
    b->len = b->cap = 0;
    b->err = 0;
}

void
otr_util_sb_add (struct t_otr_sbuf *b, const char *p, size_t n)
{
    size_t nc;
    char *t;

    if (b->err)
        return;
    if (b->len + n + 1 > b->cap)
    {
        nc = b->cap ? b->cap : 64;
        while (nc < b->len + n + 1)
            nc *= 2;
        t = realloc (b->s, nc);
        if (!t)
        {
            b->err = 1;
            return;
        }
        b->s = t;
        b->cap = nc;
    }
    memcpy (b->s + b->len, p, n);
    b->len += n;
    b->s[b->len] = '\0';
}

void
otr_util_sb_adds (struct t_otr_sbuf *b, const char *s)
{
    otr_util_sb_add (b, s, strlen (s));
}

void
otr_util_sb_addc (struct t_otr_sbuf *b, char c)
{
    otr_util_sb_add (b, &c, 1);
}

/* returns malloc'd string, or NULL on allocation failure */
char *
otr_util_sb_take (struct t_otr_sbuf *b)
{
    char *r;

    if (b->err)
    {
        free (b->s);
        b->s = NULL;
        return NULL;
    }
    if (!b->s)
        return strdup ("");
    r = b->s;
    b->s = NULL;
    return r;
}

/* replace CR, LF, NUL-ish control characters that could break an IRC line */
char *
otr_util_irc_sanitize (const char *s)
{
    char *r = otr_util_dupstr (s), *p;

    if (!r)
        return NULL;
    for (p = r; *p; p++)
        if (*p == '\r' || *p == '\n')
            *p = ' ';
    return r;
}
