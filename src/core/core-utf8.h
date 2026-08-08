/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_UTF8_H
#define WEECHAT_UTF8_H

extern int local_utf8;

extern void utf8_init (void);
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
