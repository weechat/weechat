/*
 * Copyright (C) 2012-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_EVAL_H
#define WEECHAT_EVAL_H

#include <regex.h>

#define EVAL_STR_FALSE      "0"
#define EVAL_STR_TRUE       "1"

#define EVAL_DEFAULT_PREFIX "${"
#define EVAL_DEFAULT_SUFFIX "}"

#define EVAL_RECURSION_MAX  32

#define EVAL_RANGE_DIGIT    "0123456789"
#define EVAL_RANGE_XDIGIT   EVAL_RANGE_DIGIT "abcdefABCDEF"
#define EVAL_RANGE_LOWER    "abcdefghijklmnopqrstuvwxyz"
#define EVAL_RANGE_UPPER    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define EVAL_RANGE_ALPHA    EVAL_RANGE_LOWER EVAL_RANGE_UPPER
#define EVAL_RANGE_ALNUM    EVAL_RANGE_ALPHA EVAL_RANGE_DIGIT

#define EVAL_SYNTAX_HL_MARKER  "\xef\xbf\xbf\xef\xbf\xbf"
#define EVAL_SYNTAX_HL_INC     (EVAL_SYNTAX_HL_MARKER "+")
#define EVAL_SYNTAX_HL_DEC     (EVAL_SYNTAX_HL_MARKER "-")

struct t_hashtable;

enum t_eval_logical_op
{
    EVAL_LOGICAL_OP_OR = 0,
    EVAL_LOGICAL_OP_AND,
    /* number of comparison strings */
    EVAL_NUM_LOGICAL_OPS,
};

enum t_eval_comparison
{
    EVAL_COMPARE_REGEX_MATCHING = 0,
    EVAL_COMPARE_REGEX_NOT_MATCHING,
    EVAL_COMPARE_STRING_MATCHING_CASE_SENSITIVE,
    EVAL_COMPARE_STRING_NOT_MATCHING_CASE_SENSITIVE,
    EVAL_COMPARE_STRING_MATCHING,
    EVAL_COMPARE_STRING_NOT_MATCHING,
    EVAL_COMPARE_INCLUDE_CASE_SENSITIVE,
    EVAL_COMPARE_NOT_INCLUDE_CASE_SENSITIVE,
    EVAL_COMPARE_INCLUDE,
    EVAL_COMPARE_NOT_INCLUDE,
    EVAL_COMPARE_EQUAL,
    EVAL_COMPARE_NOT_EQUAL,
    EVAL_COMPARE_LESS_EQUAL,
    EVAL_COMPARE_LESS,
    EVAL_COMPARE_GREATER_EQUAL,
    EVAL_COMPARE_GREATER,
    /* number of comparison strings */
    EVAL_NUM_COMPARISONS,
};

struct t_eval_regex
{
    const char *result;
    regmatch_t match[100];
    int last_match;
};

struct t_eval_context
{
    struct t_hashtable *pointers;      /* pointers used in eval             */
    struct t_hashtable *extra_vars;    /* extra variables used in eval      */
    struct t_hashtable *user_vars;     /* user-defined variables            */
    int extra_vars_eval;               /* 1 if extra vars must be evaluated */
    const char *prefix;                /* prefix (default is "${")          */
    int length_prefix;                 /* length of prefix                  */
    const char *suffix;                /* suffix (default is "}")           */
    int length_suffix;                 /* length of suffix                  */
    struct t_eval_regex *regex;        /* in case of replace with regex     */
    int regex_replacement_index;       /* replacement index (≥ 1)           */
    int recursion_count;               /* to prevent infinite recursion     */
    int syntax_highlight;              /* syntax highlight: ${raw_hl:...}   */
                                       /* or ${hl:...}                      */
    int debug_level;                   /* 0: no debug, 1: debug, 2: extra   */
    int debug_depth;                   /* used for debug indentation        */
    int debug_id;                      /* operation id in debug output      */
    char **debug_output;               /* string with debug output          */
};

extern int eval_is_true (const char *value);
extern char *eval_expression (const char *expr,
                              struct t_hashtable *pointers,
                              struct t_hashtable *extra_vars,
                              struct t_hashtable *options);

#endif /* WEECHAT_EVAL_H */
