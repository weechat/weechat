/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

#ifndef __WEECHAT_TRIGGER_LIBC_H
#define __WEECHAT_TRIGGER_LIBC_H 1

int c_is_number (char *);
int c_to_number (char *);
char *c_strndup (char *, int);
void c_ascii_tolower (char *);
void c_ascii_toupper (char *);
char *c_weechat_strreplace (char *, char *, char *);
char **c_explode_string (char *, char *, int,  int *);
void c_free_exploded_string (char **);
char **c_split_multi_command (char *, char);
void c_free_multi_command (char **);
char *c_join_string(char **, char *);
void c_free_joined_string (char *);
int c_match_string (char *, char *);
int c_imatch_string (char *, char *);

#endif /* trigger-libc.h */
