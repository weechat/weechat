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


#ifndef __WEECHAT_UTF8_H
#define __WEECHAT_UTF8_H 1

extern int local_utf8;

extern void utf8_init ();
extern int utf8_has_8bits (char *);
extern int utf8_is_valid (char *, char **);
extern void utf8_normalize (char *, char);
extern char *utf8_prev_char (char *, char *);
extern char *utf8_next_char (char *);
extern int utf8_char_size (char *);
extern int utf8_strlen (char *);
extern int utf8_strnlen (char *, int);
extern int utf8_width_screen (char *);
extern char *utf8_add_offset (char *, int);
extern int utf8_real_pos (char *, int);
extern int utf8_pos (char *, int);

#endif /* utf8.h */
