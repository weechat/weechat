/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

extern char *string_strndup (char *string, int length);
extern void string_tolower (char *string);
extern void string_toupper (char *string);
extern int string_strcasecmp (char *string1, char *string2);
extern int string_strncasecmp (char *string1, char *string2, int max);
extern int string_strcmp_ignore_chars (char *string1, char *string2,
                                       char *chars_ignored, int case_sensitive);
extern char *string_strcasestr (char *string, char *search);
extern int string_match (char *string, char *mask, int case_sensitive);
extern char *string_replace (char *string, char *search, char *replace);
extern char *string_remove_quotes (char *string, char *quotes);
extern char *string_strip (char *string, int left, int right, char *chars);
extern char *string_convert_hex_chars (char *string);
extern int string_has_highlight (char *string, char *highlight_words);
extern char **string_explode (char *string, char *separators, int keep_eol,
                              int num_items_max, int *num_items);
extern void string_free_exploded (char **exploded_string);
extern char *string_build_with_exploded (char **exploded_string,
                                         char *separator);
extern char **string_split_command (char *command, char separator);
extern void string_free_splitted_command (char **splitted_command);
extern char *string_iconv (int from_utf8, char *from_code, char *to_code,
                           char *string);
extern char *string_iconv_to_internal (char *charset, char *string);
extern char *string_iconv_from_internal (char *charset, char *string);
extern void string_iconv_fprintf (FILE *file, char *data, ...);

#endif /* wee-string.h */
