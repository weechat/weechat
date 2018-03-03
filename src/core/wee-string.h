/*
 * Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_STRING_H
#define WEECHAT_STRING_H

#include <stdio.h>
#include <stdint.h>

#ifdef HAVE_PCRE
#include <pcreposix.h>
#else
#include <regex.h>
#endif

typedef uint32_t string_shared_count_t;

typedef uint32_t string_dyn_size_t;
struct t_string_dyn
{
    char *string;                      /* the string                        */
    string_dyn_size_t size_alloc;      /* allocated size                    */
    string_dyn_size_t size;            /* size of string (including '\0')   */
};

struct t_hashtable;

extern char *string_strndup (const char *string, int length);
extern char *string_cut (const char *string, int length, int count_suffix,
                         int screen, const char *cut_suffix);
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
extern const char *string_strcasestr (const char *string, const char *search);
extern int string_match (const char *string, const char *mask,
                         int case_sensitive);
extern char *string_replace (const char *string, const char *search,
                             const char *replace);
extern char *string_expand_home (const char *path);
extern char *string_eval_path_home (const char *path,
                                    struct t_hashtable *pointers,
                                    struct t_hashtable *extra_vars,
                                    struct t_hashtable *options);
extern char *string_remove_quotes (const char *string, const char *quotes);
extern char *string_strip (const char *string, int left, int right,
                           const char *chars);
extern char *string_convert_escaped_chars (const char *string);
extern int string_is_word_char_highlight (const char *string);
extern int string_is_word_char_input (const char *string);
extern char *string_mask_to_regex (const char *mask);
extern const char *string_regex_flags (const char *regex, int default_flags,
                                       int *flags);
extern int string_regcomp (void *preg, const char *regex, int default_flags);
extern int string_has_highlight (const char *string,
                                 const char *highlight_words);
extern int string_has_highlight_regex_compiled (const char *string,
                                                regex_t *regex);
extern int string_has_highlight_regex (const char *string, const char *regex);
extern char *string_replace_regex (const char *string, void *regex,
                                   const char *replace,
                                   const char reference_char,
                                   char *(*callback)(void *data, const char *text),
                                   void *callback_data);
extern char **string_split (const char *string, const char *separators,
                            int keep_eol, int num_items_max, int *num_items);
extern char **string_split_shared (const char *string, const char *separators,
                                   int keep_eol, int num_items_max,
                                   int *num_items);
extern char **string_split_shell (const char *string, int *num_items);
extern void string_free_split (char **split_string);
extern void string_free_split_shared (char **split_string);
extern char *string_build_with_split_string (const char **split_string,
                                             const char *separator);
extern char **string_split_command (const char *command, char separator);
extern void string_free_split_command (char **split_command);
extern char *string_iconv (int from_utf8, const char *from_code,
                           const char *to_code, const char *string);
extern char *string_iconv_to_internal (const char *charset, const char *string);
extern char *string_iconv_from_internal (const char *charset,
                                         const char *string);
extern int string_fprintf (FILE *file, const char *data, ...);
extern char *string_format_size (unsigned long long size);
extern void string_encode_base16 (const char *from, int length, char *to);
extern int string_decode_base16 (const char *from, char *to);
extern void string_encode_base64 (const char *from, int length, char *to);
extern int string_decode_base64 (const char *from, char *to);
extern char *string_hex_dump (const char *data, int data_size,
                              int bytes_per_line,
                              const char *prefix, const char *suffix);
extern int string_is_command_char (const char *string);
extern const char *string_input_for_buffer (const char *string);
extern char *string_replace_with_callback (const char *string,
                                           const char *prefix,
                                           const char *suffix,
                                           const char **list_prefix_no_replace,
                                           char *(*callback)(void *data, const char *text),
                                           void *callback_data,
                                           int *errors);
extern const char *string_shared_get (const char *string);
extern void string_shared_free (const char *string);
extern char **string_dyn_alloc (int size_alloc);
extern int string_dyn_copy (char **string, const char *new_string);
extern int string_dyn_concat (char **string, const char *add);
extern char *string_dyn_free (char **string, int free_string);
extern void string_end ();

#endif /* WEECHAT_STRING_H */
