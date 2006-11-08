/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_UTIL_H
#define __WEECHAT_UTIL_H 1

#ifndef HAVE_STRNDUP
extern char *strndup (char *, int);
#endif
extern void ascii_tolower (char *);
extern void ascii_toupper (char *);
extern int ascii_strcasecmp (char *, char *);
extern int ascii_strncasecmp (char *, char *, int);
extern char *ascii_strcasestr (char *, char *);
extern char *weechat_iconv (char *, char *, char *);
extern char *weechat_iconv_to_internal (char *, char *);
extern char *weechat_iconv_from_internal (char *, char *);
extern void weechat_iconv_fprintf (FILE *, char *, ...);
extern char *weechat_strreplace (char *, char *, char *);
extern long get_timeval_diff (struct timeval *, struct timeval *);
extern char **explode_string (char *, char *, int, int *);
extern void free_exploded_string (char **);

#endif /* util.h */
