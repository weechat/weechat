/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_UTIL_H
#define WEECHAT_PLUGIN_OTR_UTIL_H

struct sbuf
{
    char *s;
    size_t len, cap;
    int err;
};

extern char *strf (const char *fmt, ...);
extern char *dupstr (const char *s);
extern char *lc_dup (const char *s);
extern void sb_init (struct sbuf *b);
extern void sb_add (struct sbuf *b, const char *p, size_t n);
extern void sb_adds (struct sbuf *b, const char *s);
extern void sb_addc (struct sbuf *b, char c);
extern char *sb_take (struct sbuf *b);
extern char *irc_sanitize (const char *s);

#endif /* WEECHAT_PLUGIN_OTR_UTIL_H */
