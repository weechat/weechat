/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_UTIL_H
#define WEECHAT_PLUGIN_OTR_UTIL_H

struct t_otr_sbuf
{
    char *s;
    size_t len, cap;
    int err;
};

extern char *otr_util_strf (const char *fmt, ...);
extern char *otr_util_dupstr (const char *s);
extern char *otr_util_lc_dup (const char *s);
extern void otr_util_sb_init (struct t_otr_sbuf *b);
extern void otr_util_sb_add (struct t_otr_sbuf *b, const char *p, size_t n);
extern void otr_util_sb_adds (struct t_otr_sbuf *b, const char *s);
extern void otr_util_sb_addc (struct t_otr_sbuf *b, char c);
extern char *otr_util_sb_take (struct t_otr_sbuf *b);
extern char *otr_util_irc_sanitize (const char *s);

#endif /* WEECHAT_PLUGIN_OTR_UTIL_H */
