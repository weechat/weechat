/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_STRING_H
#define __WEECHAT_STRING_H 1

#ifndef HAVE_STRNDUP
extern char *strndup (char *, int);
#endif
extern void string_tolower (char *);
extern void string_toupper (char *);
extern int string_strcasecmp (char *, char *);
extern int string_strncasecmp (char *, char *, int);
extern char *string_strcasestr (char *, char *);
extern char *string_replace (char *, char *, char *);
extern char *string_convert_hex_chars (char *);
extern char **string_explode (char *, char *, int, int, int *);
extern void string_free_exploded (char **);
extern char **string_split_multi_command (char *, char);
extern void string_free_multi_command (char **);
extern char *string_iconv (int, char *, char *, char *);
extern char *string_iconv_to_internal (char *, char *);
extern char *string_iconv_from_internal (char *, char *);
extern void string_iconv_fprintf (FILE *, char *, ...);

#endif /* wee-string.h */
