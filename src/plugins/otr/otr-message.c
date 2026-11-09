/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Conversion of OTR message text */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-message.h"
#include "otr-util.h"


int
is_ctcp (const char *t)
{
    size_t l = strlen (t);

    return l >= 2 && t[0] == '\x01' && t[l - 1] == '\x01';
}

int
is_action (const char *t)
{
    return is_ctcp (t) && strncmp (t, "\x01" "ACTION ", 8) == 0;
}

/* "\x01ACTION text\x01" -> "/me text"; otherwise plain copy */
char *
msg_plain_from_irc (const char *t)
{
    size_t l;

    if (!is_action (t))
        return dupstr (t);
    l = strlen (t);
    return strf (ACTION_PREFIX "%.*s", (int)(l - 9), t + 8);
}

char *
html_escape (const char *in)
{
    struct sbuf b;

    sb_init (&b);
    for (; *in; in++)
    {
        if (*in == '&')
            sb_adds (&b, "&amp;");
        else if (*in == '<')
            sb_adds (&b, "&lt;");
        else if (*in == '>')
            sb_adds (&b, "&gt;");
        else
            sb_addc (&b, *in);
    }
    return sb_take (&b);
}

static void
utf8_append (struct sbuf *b, unsigned long cp)
{
    char t[4];

    if (cp == 0)
        return;
    if (cp < 0x80)
    {
        t[0] = (char)cp;
        sb_add (b, t, 1);
    }
    else if (cp < 0x800)
    {
        t[0] = (char)(0xC0 | (cp >> 6));
        t[1] = (char)(0x80 | (cp & 0x3F));
        sb_add (b, t, 2);
    }
    else if (cp < 0x10000)
    {
        t[0] = (char)(0xE0 | (cp >> 12));
        t[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        t[2] = (char)(0x80 | (cp & 0x3F));
        sb_add (b, t, 3);
    }
    else if (cp <= 0x10FFFF)
    {
        t[0] = (char)(0xF0 | (cp >> 18));
        t[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        t[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        t[3] = (char)(0x80 | (cp & 0x3F));
        sb_add (b, t, 4);
    }
}

/* Reduce HTML to IRC text: keep text, <br> -> newline, links -> [text](url),
 * decode a few entities, drop all other tags. */
char *
html_filter (const char *in)
{
    struct sbuf b;
    char link[1024];
    size_t linkstart = 0;
    const char *p = in, *gt, *semi, *h;
    char name[16];
    int closing, k;

    link[0] = '\0';
    sb_init (&b);
    while (*p)
    {
        if (*p == '<' && (isalpha ((unsigned char)p[1]) || p[1] == '/'
                          || p[1] == '!')
            && (gt = strchr (p, '>')) != NULL)
        {
            const char *t = p + 1;
            closing = 0;
            if (*t == '/')
            {
                closing = 1;
                t++;
            }
            for (k = 0; k < (int)sizeof (name) - 1 && isalpha ((unsigned char)t[k]);
                 k++)
                name[k] = (char)tolower ((unsigned char)t[k]);
            name[k] = '\0';

            if (!closing && strcmp (name, "br") == 0)
                sb_addc (&b, '\n');
            else if (!closing && strcmp (name, "a") == 0)
            {
                /* look for href="..." inside the tag */
                char *content = strndup (p, (size_t)(gt - p));

                if (content)
                {
                    h = NULL;
                    for (const char *q = content; *q; q++)
                        if (strncasecmp (q, "href=", 5) == 0)
                        {
                            h = q + 5;
                            break;
                        }
                    if (h)
                    {
                        char quote = (*h == '"' || *h == '\'') ? *h : 0;
                        size_t n = 0;

                        if (quote)
                            h++;
                        while (h[n] && (quote ? h[n] != quote
                                              : !isspace ((unsigned char)h[n]))
                               && n < sizeof (link) - 1)
                            n++;
                        memcpy (link, h, n);
                        link[n] = '\0';
                        if (n > 0)
                        {
                            sb_addc (&b, '[');
                            linkstart = b.len;
                        }
                    }
                    free (content);
                }
            }
            else if (closing && strcmp (name, "a") == 0 && link[0])
            {
                size_t ll = strlen (link);

                if (b.s && b.len - linkstart == ll
                    && memcmp (b.s + linkstart, link, ll) == 0)
                    sb_addc (&b, ']');
                else
                {
                    sb_adds (&b, "](");
                    sb_adds (&b, link);
                    sb_addc (&b, ')');
                }
                link[0] = '\0';
            }
            p = gt + 1;
        }
        else if (*p == '&' && (semi = strchr (p, ';')) != NULL
                 && semi - p <= 10 && semi - p > 1)
        {
            size_t l = (size_t)(semi - p - 1);
            char ent[16];

            memcpy (ent, p + 1, l);
            ent[l] = '\0';
            if (ent[0] == '#')
            {
                char *end;
                unsigned long cp = (ent[1] == 'x' || ent[1] == 'X')
                    ? strtoul (ent + 2, &end, 16) : strtoul (ent + 1, &end, 10);

                if (*end == '\0' && l > 1 && cp > 0)
                    utf8_append (&b, cp);
                else
                    sb_add (&b, p, (size_t)(semi - p) + 1);
            }
            else if (strcmp (ent, "amp") == 0)
                sb_addc (&b, '&');
            else if (strcmp (ent, "lt") == 0)
                sb_addc (&b, '<');
            else if (strcmp (ent, "gt") == 0)
                sb_addc (&b, '>');
            else if (strcmp (ent, "quot") == 0)
                sb_addc (&b, '"');
            else if (strcmp (ent, "apos") == 0)
                sb_addc (&b, '\'');
            else if (strcmp (ent, "nbsp") == 0)
                sb_addc (&b, ' ');
            else
                sb_add (&b, p, (size_t)(semi - p) + 1);
            p = semi + 1;
        }
        else
        {
            sb_addc (&b, *p);
            p++;
        }
    }
    return sb_take (&b);
}

/* Add one decrypted line as an IRC protocol line to out. Removes CTCP
 * markers a peer could smuggle in; only a well-formed /me becomes ACTION. */
static void
add_decrypted_line (struct sbuf *out, const char *tags, const char *from,
                    const char *target, const char *line, size_t len)
{
    struct sbuf body;
    const char *p = line, *end = line + len;
    int action = 0;
    char *text;

    if (len >= 4 && strncmp (line, ACTION_PREFIX, 4) == 0)
    {
        action = 1;
        p += 4;
    }
    else if (len >= 9 && strncmp (line, "\x01" "ACTION ", 8) == 0
             && line[len - 1] == '\x01')
    {
        action = 1;
        p += 8;
        end--;
    }

    sb_init (&body);
    for (; p < end; p++)
        if (*p != '\x01' && *p != '\r' && *p != '\0')
            sb_addc (&body, *p);
    text = sb_take (&body);
    if (!text)
    {
        out->err = 1;
        return;
    }

    if (out->len)
        sb_adds (out, "\r\n");
    sb_adds (out, tags);
    sb_addc (out, ':');
    sb_adds (out, from);
    sb_adds (out, " PRIVMSG ");
    sb_adds (out, target);
    sb_adds (out, " :");
    if (action)
        sb_adds (out, "\x01" "ACTION ");
    sb_adds (out, text);
    if (action)
        sb_addc (out, '\x01');
    free (text);
}

/* msg may have several lines; each becomes its own PRIVMSG */
char *
build_in_privmsgs (const char *tags, const char *from, const char *target,
                   const char *msg, const char *line_prefix, int decrypted)
{
    struct sbuf out;
    const char *p = msg, *nl;
    size_t l;

    sb_init (&out);
    while (*p)
    {
        nl = strchr (p, '\n');
        l = nl ? (size_t)(nl - p) : strlen (p);
        if (l && p[l - 1] == '\r')
            l--;
        if (decrypted)
            add_decrypted_line (&out, tags, from, target, p, l);
        else
        {
            struct sbuf ln;
            char *t;

            sb_init (&ln);
            sb_adds (&ln, line_prefix);
            sb_add (&ln, p, l);
            t = sb_take (&ln);
            if (t)
            {
                if (out.len)
                    sb_adds (&out, "\r\n");
                sb_adds (&out, tags);
                sb_addc (&out, ':');
                sb_adds (&out, from);
                sb_adds (&out, " PRIVMSG ");
                sb_adds (&out, target);
                sb_adds (&out, " :");
                for (char *q = t; *q; q++)
                    if (*q != '\r')
                        sb_addc (&out, *q);
                free (t);
            }
        }
        p = nl ? nl + 1 : p + strlen (p);
    }
    return sb_take (&out);
}
