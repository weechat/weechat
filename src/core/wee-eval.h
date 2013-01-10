/*
 * Copyright (C) 2012-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_EVAL_H
#define __WEECHAT_EVAL_H 1

#define EVAL_STR_FALSE "0"
#define EVAL_STR_TRUE  "1"

struct t_hashtable;

enum t_eval_logical_op
{
    EVAL_LOGICAL_OP_AND = 0,
    EVAL_LOGICAL_OP_OR,
    /* number of comparison strings */
    EVAL_NUM_LOGICAL_OPS,
};

enum t_eval_comparison
{
    EVAL_COMPARE_EQUAL = 0,
    EVAL_COMPARE_NOT_EQUAL,
    EVAL_COMPARE_LESS_EQUAL,
    EVAL_COMPARE_LESS,
    EVAL_COMPARE_GREATER_EQUAL,
    EVAL_COMPARE_GREATER,
    EVAL_COMPARE_REGEX_MATCHING,
    EVAL_COMPARE_REGEX_NOT_MATCHING,
    /* number of comparison strings */
    EVAL_NUM_COMPARISONS,
};

extern int eval_is_true (const char *value);
extern char *eval_expression (const char *expr,
                              struct t_hashtable *pointers,
                              struct t_hashtable *extra_vars);

#endif /* __WEECHAT_EVAL_H */
