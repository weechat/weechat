/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_STRING_H
#define __WEECHAT_STRING_H 1

#include <regex.h>

struct t_hashtable;

extern char *string_strndup (const char *string, int length);
extern void string_tolower (char *string);
extern void string_toupper (char *string);
extern int string_strcasecmp (const char *string1, const char *string2);
extern int string_strcasecmp_range (const char *string1, const char *string2,
                                    int range);
extern int string_strncasecmp (const char *string1, const char *string2,
                               int max);
extern int string_strncasecmp_range (const char *string1, const char *string2,
                                     int max, int range);
extern int string_strcmp_ignore_chars (const char *string1,
                                       const char *string2,
                                       const char *chars_ignored,
                                       int case_sensitive);
extern char *string_strcasestr (const char *string, const char *search);
extern int string_match (const char *string, const char *mask,
                         int case_sensitive);
extern char *string_replace (const char *string, const char *search,
                             const char *replace);
extern char *string_expand_home (const char *path);
extern char *string_remove_quotes (const char *string, const char *quotes);
extern char *string_strip (const char *string, int left, int right,
                           const char *chars);
extern char *string_convert_hex_chars (const char *string);
extern char *string_mask_to_regex (const char *mask);
extern const char *string_regex_flags (const char *regex, int default_flags,
                                       int *flags);
extern int string_regcomp (void *preg, const char *regex, int default_flags);
extern int string_has_highlight (const char *string,
                                 const char *highlight_words);
extern int string_has_highlight_regex_compiled (const char *string,
                                                regex_t *regex);
extern int string_has_highlight_regex (const char *string, const char *regex);
extern char **string_split (const char *string, const char *separators,
                            int keep_eol, int num_items_max, int *num_items);
extern char **string_split_shell (const char *string);
extern void string_free_split (char **split_string);
extern char *string_build_with_split_string (const char **split_string,
                                             const char *separator);
extern char **string_split_command (const char *command, char separator);
extern void string_free_split_command (char **split_command);
extern char *string_iconv (int from_utf8, const char *from_code,
                           const char *to_code, const char *string);
extern char *string_iconv_to_internal (const char *charset, const char *string);
extern char *string_iconv_from_internal (const char *charset,
                                         const char *string);
extern int string_iconv_fprintf (FILE *file, const char *data, ...);
extern char *string_format_size (unsigned long long size);
extern void string_encode_base64 (const char *from, int length, char *to);
extern int string_decode_base64 (const char *from, char *to);
extern int string_is_command_char (const char *string);
extern const char *string_input_for_buffer (const char *string);
extern char *string_replace_with_callback (const char *string,
                                           char *(*callback)(void *data, const char *text),
                                           void *callback_data,
                                           int *errors);

#endif /* __WEECHAT_STRING_H */
