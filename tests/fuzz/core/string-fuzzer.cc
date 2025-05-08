/*
 * SPDX-FileCopyrightText: 2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Fuzz testing on WeeChat core string functions */

extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <regex.h>

#include "src/core/core-config.h"
#include "src/core/core-string.h"
#include "src/plugins/weechat-plugin.h"
}

regex_t global_regex;

extern "C" int
LLVMFuzzerInitialize (int *argc, char ***argv)
{
    /* make C++ compiler happy */
    (void) argc;
    (void) argv;

    string_init ();
    config_weechat_init ();

    regcomp (&global_regex, "a.*", 0);

    return 0;
}

char *
callback_replace (void *data, const char *text)
{
    /* make C++ compiler happy */
    (void) data;
    (void) text;

    return strdup ("z");
}

extern "C" int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
    const char *masks[3] = { "a*", "b*", NULL};
    char *str, *str2, **str_dyn;
    char **argv, *buffer;
    const char *name;
    int argc, flags, num_tags, priority;
    regex_t regex;

    str = (char *)malloc (size + 1);
    memcpy (str, data, size);
    str[size] = '\0';

    free (string_strndup (str, size / 2));

    free (string_cut (str, size / 2, 0, 0, "…"));
    free (string_cut (str, size / 2, 1, 0, "…"));
    free (string_cut (str, size / 2, 0, 1, "…"));
    free (string_cut (str, size / 2, 1, 1, "…"));
    free (string_cut (str, size / 2, 1, 1, NULL));

    free (string_reverse (str));

    free (string_reverse_screen (str));

    free (string_repeat (str, 2));

    free (string_tolower (str));

    free (string_toupper (str));

    free (string_tolower_range (str, 13));

    free (string_toupper_range (str, 13));

    string_strcmp (str, str);

    string_strncmp (str, str, size / 2);

    string_strcasecmp (str, str);

    string_strcasecmp_range (str, str, 13);

    string_strncasecmp (str, str, size / 2);

    string_strncasecmp_range (str, str, size / 2, 13);

    string_strcmp_ignore_chars (str, str, "abcd", 0);
    string_strcmp_ignore_chars (str, str, "abcd", 1);

    string_strcasestr (str, str);

    string_match (str, "a*", 0);
    string_match (str, "a*", 1);
    string_match (str, "*b", 0);
    string_match (str, "*b", 1);
    string_match (str, "*c*", 0);
    string_match (str, "*c*", 1);

    string_match_list (str, (const char **)masks, 0);
    string_match_list (str, (const char **)masks, 1);

    free (string_expand_home (str));

    free (string_eval_path_home (str, NULL, NULL, NULL));

    free (string_remove_quotes (str, "'\""));

    free (string_strip (str, 0, 0, "abcdef"));
    free (string_strip (str, 0, 1, "abcdef"));
    free (string_strip (str, 1, 0, "abcdef"));
    free (string_strip (str, 1, 1, "abcdef"));

    free (string_convert_escaped_chars (str));

    string_is_whitespace_char (str);

    string_is_word_char_highlight (str);

    string_is_word_char_input (str);

    free (string_mask_to_regex (str));

    string_regex_flags (str, 0, NULL);
    string_regex_flags (str, 0, &flags);

    if (string_regcomp (&regex, str, REG_ICASE | REG_NOSUB) == 0)
        regfree (&regex);
    str2 = (char *)malloc (16 + size + 1);
    snprintf (str2, 16 + size + 1, "(?ins)%s", str);
    if (string_regcomp (&regex, str2, REG_ICASE | REG_NOSUB) == 0)
        regfree (&regex);
    free (str2);

    string_has_highlight (str, "a");

    string_has_highlight_regex_compiled (str, &global_regex);

    string_has_highlight_regex (str, "a.*");

    free (string_replace (str, "a", "b"));

    free (string_replace_regex (str, &global_regex, "b", '$', &callback_replace, NULL));

    free (string_translate_chars (str, "abc", "def"));

    string_free_split (string_split (str, "/", NULL, 0, 0, &argc));
    string_free_split (string_split (str, "/", " ", 0, 0, &argc));
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS;
    argv = string_split (str, "/", " ", flags, 0, &argc);
    free (string_rebuild_split_string ((const char **)argv, "/", 0, -1));
    string_free_split (argv);
    flags = WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
        | WEECHAT_STRING_SPLIT_KEEP_EOL;
    string_free_split (string_split (str, "/", " ", flags, 0, &argc));

    string_free_split_shared (string_split_shared (str, "/", " ", flags, 0, &argc));

    string_free_split (string_split_shell (str, &argc));

    string_free_split_command (string_split_command (str, ';'));

    string_free_split_tags (string_split_tags (str, &num_tags));

    free (string_iconv (0, "utf-8", "iso-8859-1", str));

    free (string_iconv_to_internal ("iso-8859-1", str));

    free (string_iconv_from_internal ("iso-8859-1", str));

    string_parse_size (str);

    buffer = (char *)malloc ((size * 4) + 8 + 1);
    string_base16_encode (str, size, buffer);
    string_base16_decode (str, buffer);
    string_base_encode ("16", str, size, buffer);
    string_base_decode ("16", str, buffer);
    string_base32_encode (str, size, buffer);
    string_base32_decode (str, buffer);
    string_base_encode ("32", str, size, buffer);
    string_base_decode ("32", str, buffer);
    string_base64_encode (0, str, size, buffer);
    string_base64_decode (0, str, buffer);
    string_base_encode ("64", str, size, buffer);
    string_base_decode ("64", str, buffer);
    string_base64_encode (1, str, size, buffer);
    string_base64_decode (1, str, buffer);
    string_base_encode ("64url", str, size, buffer);
    string_base_decode ("64url", str, buffer);
    free (buffer);

    free (string_hex_dump (str, size, 16, "<", ">"));

    string_is_command_char (str);

    string_input_for_buffer (str);

    string_get_common_bytes_count (str, str);

    string_levenshtein (str, str, 0);
    string_levenshtein (str, str, 1);

    string_get_priority_and_name (str, &priority, &name, 0);

    string_shared_free ((char *)string_shared_get (str));

    str_dyn = string_dyn_alloc (1);
    string_dyn_copy (str_dyn, str);
    string_dyn_concat (str_dyn, str, -1);
    string_dyn_free (str_dyn, 1);

    string_concat ("/", str, str, NULL);

    free (str);

    return 0;
}
