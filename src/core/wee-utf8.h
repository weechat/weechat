/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_UTF8_H
#define WEECHAT_UTF8_H

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <wchar.h>

extern int local_utf8;

extern void utf8_init ();
extern int utf8_has_8bits (const char *string);
extern int utf8_is_valid (const char *string, int length, char **error);
extern void utf8_normalize (char *string, char replacement);
extern const char *utf8_prev_char (const char *string_start,
                                   const char *string);
extern const char *utf8_next_char (const char *string);
extern const char *utf8_beginning_of_line (const char *string_start,
                                           const char *string);
extern const char *utf8_end_of_line (const char *string);
extern int utf8_char_int (const char *string);
extern int utf8_int_string (unsigned int unicode_value, char *string);
extern int utf8_char_size (const char *string);
extern int utf8_strlen (const char *string);
extern int utf8_strnlen (const char *string, int bytes);
extern int utf8_strlen_screen (const char *string);
extern int utf8_char_size_screen (const char *string);
extern const char *utf8_add_offset (const char *string, int offset);
extern int utf8_real_pos (const char *string, int pos);
extern int utf8_pos (const char *string, int real_pos);
extern char *utf8_strndup (const char *string, int length);
extern void utf8_strncpy (char *dest, const char *string, int length);

#endif /* WEECHAT_UTF8_H */
